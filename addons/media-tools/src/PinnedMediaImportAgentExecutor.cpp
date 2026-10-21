#include "PinnedMediaImportAgentExecutor.h"

#include "AddonRootApproval.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace vdrsuite::mediatools {
namespace {
std::int64_t unixNow()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool safePath(const std::filesystem::path& path)
{
    if (!path.is_absolute() || path.empty() ||
        path.string().size() > 4096 ||
        path.string().find('\0') != std::string::npos)
        return false;
    for (const auto& part : path)
        if (part == "." || part == "..") return false;
    return true;
}

// Execute the *pinned descriptor*, never reopen the installed pathname.
// Do not inherit the Agent's environment or leak API payloads to logs.
// A killed or failed child is always reconciliation-required (unknown).
MediaImportAgentOutcome runPinned(
    int fd, const LocalMediaImportSource& source, std::int64_t deadline)
{
    if (fd < 0 || !safePath(source.sourceRoot) ||
        !safePath(source.videoRoot) || !safePath(source.sourceFile))
        return MediaImportAgentOutcome::rejected;
    const auto now = unixNow();
    if (deadline <= now || deadline > now + 3600)
        return MediaImportAgentOutcome::rejected;

    std::string executable = "vdr-suite-media-import";
    std::string action = "import";
    std::string sourceRoot = source.sourceRoot.string();
    std::string videoRoot = source.videoRoot.string();
    std::string sourceFile = source.sourceFile.string();
    std::string confirmation = "--confirm-writes";
    char* const argv[] = {
        executable.data(), action.data(), sourceRoot.data(), videoRoot.data(),
        sourceFile.data(), confirmation.data(), nullptr
    };
    char environmentPath[] = "PATH=/usr/bin:/bin";
    char environmentLocale[] = "LC_ALL=C";
    char* const environment[] = {environmentPath, environmentLocale, nullptr};

    const pid_t pid = ::fork();
    if (pid < 0) return MediaImportAgentOutcome::unknown;
    if (pid == 0)
    {
        // New process group contains spawned ffmpeg/ffprobe descendants.
        if (::setsid() < 0) ::_exit(127);
        const int devnull = ::open("/dev/null", O_WRONLY | O_CLOEXEC);
        if (devnull < 0 || ::dup2(devnull, STDOUT_FILENO) < 0 ||
            ::dup2(devnull, STDERR_FILENO) < 0)
            ::_exit(127);
        ::close(devnull);
        ::fexecve(fd, argv, environment);
        ::_exit(127);
    }

    // Bound child lifetime; the Agent cannot turn a deadline into a blind
    // replay. Even exit 0 only means submitted, never recording verified.
    const auto until = std::chrono::steady_clock::now() +
        std::chrono::seconds(std::min<std::int64_t>(deadline - now, 600));
    int status = 0;
    while (std::chrono::steady_clock::now() < until)
    {
        const pid_t waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid)
            return WIFEXITED(status) && WEXITSTATUS(status) == 0
                ? MediaImportAgentOutcome::submitted
                : MediaImportAgentOutcome::unknown;
        if (waited < 0) return MediaImportAgentOutcome::unknown;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ::kill(-pid, SIGKILL);
    ::kill(pid, SIGKILL);
    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    return MediaImportAgentOutcome::unknown;
}
} // namespace

PinnedMediaImportAgentExecutor::PinnedMediaImportAgentExecutor(
    std::string approvalRoot, std::string backendId,
    std::uint64_t backendGeneration, ILocalMediaImportInventory& inventory,
    ILocalMediaImportAuthorization& authorization)
    : root_(std::move(approvalRoot)),
      backendId_(std::move(backendId)),
      generation_(backendGeneration),
      inventory_(inventory),
      authorization_(authorization)
{
}

bool PinnedMediaImportAgentExecutor::available()
{
    // This read never authorizes execution. A fresh pinned read at authorize()
    // and a root-owned approval recheck at execute() are still required.
    return !backendId_.empty() && generation_ != 0 &&
        vdrsuite::addons::inspectMediaToolsInstallSnapshot(root_).bytesVerified();
}

bool PinnedMediaImportAgentExecutor::authorize(
    const BackendAgentCommandAssignment& assignment,
    const std::string& backendId, std::uint64_t generation)
{
    pinned_.reset();
    pendingCommandId_.clear();
    pendingFingerprint_.clear();
    std::string sourceId;
    if (backendId != backendId_ || generation != generation_ ||
        assignment.backendId != backendId_ ||
        assignment.backendGeneration != generation_ ||
        assignment.deadline <= unixNow() ||
        assignment.verificationPolicy != "reconciliation_required" ||
        assignment.commandType != kMediaImportCommandType ||
        !backendAgentCommandValidAssignment(assignment) ||
        !parseMediaImportSelection(assignment.payload, sourceId) ||
        !authorization_.granted(assignment))
        return false;

    // Never accept source/path from the assignment. Verify an inventory key.
    LocalMediaImportSource local;
    if (!inventory_.resolve(backendId_, sourceId, local) ||
        !safePath(local.sourceRoot) ||
        !safePath(local.videoRoot) ||
        !safePath(local.sourceFile))
        return false;

    auto snapshot = std::make_unique<vdrsuite::addons::MediaToolsInstallSnapshot>(
        vdrsuite::addons::inspectMediaToolsInstallSnapshot(root_));
    if (!snapshot->bytesVerified()) return false;
    pinned_ = std::move(snapshot);
    pendingCommandId_ = assignment.commandId;
    pendingFingerprint_ = assignment.requestFingerprint;
    return true;
}

MediaImportAgentOutcome PinnedMediaImportAgentExecutor::execute(
    const BackendAgentCommandAssignment& assignment)
{
    // One-shot permit: even direct repeated calls never run the same handler.
    auto snapshot = std::move(pinned_);
    const bool matching = snapshot && snapshot->bytesVerified() &&
        assignment.commandId == pendingCommandId_ &&
        assignment.requestFingerprint == pendingFingerprint_ &&
        assignment.backendId == backendId_ &&
        assignment.backendGeneration == generation_ &&
        assignment.deadline > unixNow();
    pendingCommandId_.clear();
    pendingFingerprint_.clear();
    if (!matching) return MediaImportAgentOutcome::rejected;
    const auto approval = vdrsuite::addons::readRootMediaToolsApproval(root_);
    if (!approval.approved() || approval.sha256 != snapshot->approvedSha256 ||
        approval.version != snapshot->approvedVersion ||
        approval.origin != snapshot->origin ||
        !authorization_.granted(assignment))
        return MediaImportAgentOutcome::rejected;

    std::string sourceId;
    LocalMediaImportSource local;
    if (!parseMediaImportSelection(assignment.payload, sourceId) ||
        !inventory_.resolve(backendId_, sourceId, local))
        return MediaImportAgentOutcome::rejected;

    return runPinned(snapshot->pinnedBinary.get(), local, assignment.deadline);
}

} // namespace vdrsuite::mediatools
