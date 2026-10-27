#include "PinnedMediaImportAgentExecutor.h"

#include "AddonRootApproval.h"
#include "AddonInstalledManifest.h"
#include "MediaImportRecordingEvidence.h"
#include "MediaToolsImportRunLock.h"
#include "MediaImportDecisionMarker.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <cstring>
#include <iomanip>
#include <sstream>
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

// The handler runs through the reviewed pinned descriptor. Its stdout is
// observed only to correlate a freshly promoted .rec with Suite's readback;
// neither exit 0 nor stdout alone verifies a recording.
std::string parseCommittedRecording(
    const std::string& stdoutText, const LocalMediaImportSource& source)
{
    const std::string marker = "RECORDING=";
    const auto pos = stdoutText.find(marker);
    if (pos == std::string::npos ||
        (pos != 0 && stdoutText[pos - 1] != '\n') ||
        stdoutText.find(marker, pos + marker.size()) != std::string::npos ||
        stdoutText.find("IMPORT_STATUS=COMMITTED\n") == std::string::npos)
        return {};
    const auto last = stdoutText.find('\n', pos);
    if (last == std::string::npos) return {};
    const std::string field =
        stdoutText.substr(pos + marker.size(),
                          last - (pos + marker.size()));
    std::string destination = field;
    // C++17 ostream << std::filesystem::path uses std::quoted. Accept that
    // precise format (including escaped quotation marks), never strip
    // delimiters loosely or reinterpret an untrusted arbitrary pathname.
    if (!field.empty() && field.front() == '"')
    {
        std::istringstream input(field);
        if (!(input >> std::quoted(destination)) ||
            input.peek() != std::char_traits<char>::eof())
            return {};
    }
    const std::filesystem::path path(destination);
    if (!safePath(path) || path.lexically_normal() != path ||
        path.extension() != ".rec")
        return {};
    const auto relative = path.lexically_relative(source.videoRoot);
    if (relative.empty() || relative == "." || relative.is_absolute())
        return {};
    for (const auto& component : relative)
        if (component == "." || component == "..") return {};
    return recordingPathEvidence(destination);
}

MediaImportAgentOutcome runPinned(
    int fd, const LocalMediaImportSource& source, std::int64_t deadline,
    MediaToolsImportRunLock& runLock, std::string& recordingEvidence,
    bool approveVideoReencode)
{
    recordingEvidence.clear();
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
    std::string encodingApproval = "--approve-video-reencode";
    char* const argv[] = {
        executable.data(), action.data(), sourceRoot.data(), videoRoot.data(),
        sourceFile.data(), confirmation.data(),
        approveVideoReencode ? encodingApproval.data() : nullptr, nullptr
    };
    char environmentPath[] = "PATH=/usr/bin:/bin";
    char environmentLocale[] = "LC_ALL=C";
    char* const environment[] = {environmentPath, environmentLocale, nullptr};

    int capture[2] = {-1, -1};
    if (::pipe(capture) != 0) return MediaImportAgentOutcome::unknown;
    (void)::fcntl(capture[0], F_SETFD, FD_CLOEXEC);
    (void)::fcntl(capture[1], F_SETFD, FD_CLOEXEC);
    if (::fcntl(capture[0], F_SETFL, O_NONBLOCK) != 0)
    {
        ::close(capture[0]);
        ::close(capture[1]);
        return MediaImportAgentOutcome::unknown;
    }
    const pid_t pid = ::fork();
    if (pid < 0)
    {
        ::close(capture[0]);
        ::close(capture[1]);
        return MediaImportAgentOutcome::unknown;
    }
    if (pid == 0)
    {
        ::close(capture[0]);
        // This shared kernel lock must live for the *handler* lifetime,
        // including ffmpeg descendants if the Agent parent crashes.
        if (!runLock.inheritInExecChild() || ::setsid() < 0) ::_exit(127);
        const int devnull = ::open("/dev/null", O_WRONLY | O_CLOEXEC);
        if (devnull < 0 ||
            ::dup2(capture[1], STDOUT_FILENO) < 0 ||
            ::dup2(devnull, STDERR_FILENO) < 0)
            ::_exit(127);
        ::close(capture[1]);
        ::close(devnull);
        ::fexecve(fd, argv, environment);
        ::_exit(127);
    }
    ::close(capture[1]);
    std::string output;
    bool overflow = false;
    const auto drain = [&]() {
        char bytes[1024];
        while (true)
        {
            const ssize_t count = ::read(capture[0], bytes, sizeof bytes);
            if (count > 0)
            {
                if (output.size() + static_cast<std::size_t>(count) <= 8192)
                    output.append(bytes, static_cast<std::size_t>(count));
                else
                    overflow = true;
                continue;
            }
            if (count < 0 && errno == EINTR) continue;
            break;
        }
    };

    const auto until = std::chrono::steady_clock::now() +
        std::chrono::seconds(std::min<std::int64_t>(deadline - now, 600));
    int status = 0;
    while (std::chrono::steady_clock::now() < until)
    {
        drain();
        const pid_t waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid)
        {
            drain();
            ::close(capture[0]);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
                return MediaImportAgentOutcome::unknown;
            if (!overflow)
                recordingEvidence = parseCommittedRecording(output, source);
            // No evidence -> still unverified, never a false success.
            return MediaImportAgentOutcome::submitted;
        }
        if (waited < 0)
        {
            ::close(capture[0]);
            return MediaImportAgentOutcome::unknown;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    ::kill(-pid, SIGKILL);
    ::kill(pid, SIGKILL);
    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    ::close(capture[0]);
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
        vdrsuite::addons::inspectMediaToolsInstallSnapshot(root_).bytesVerified() &&
        MediaToolsImportRunLock::tryAcquire(
            root_, MediaToolsImportRunLock::Mode::sharedExecution).has_value();
}

bool PinnedMediaImportAgentExecutor::authorize(
    const BackendAgentCommandAssignment& assignment,
    const std::string& backendId, std::uint64_t generation)
{
    pinned_.reset();
    recordingEvidence_.clear();
    pendingCommandId_.clear();
    pendingFingerprint_.clear();
    pendingSource_ = {};
    pendingSourceValid_ = false;
    pendingDecision_ = MediaImportDecision::import;
    markerEvidence_.clear();
    std::string sourceId;
    MediaImportDecision decision = MediaImportDecision::import;
    if (backendId != backendId_ || generation != generation_ ||
        assignment.backendId != backendId_ ||
        assignment.backendGeneration != generation_ ||
        assignment.deadline <= unixNow() ||
        assignment.verificationPolicy != "reconciliation_required" ||
        assignment.commandType != kMediaImportCommandType ||
        !backendAgentCommandValidAssignment(assignment) ||
        !parseMediaImportSelection(
            assignment.payload, sourceId, decision) ||
        assignment.payloadVersion !=
            mediaImportSelectionVersion(decision) ||
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
    pendingSource_ = local;
    pendingSourceValid_ = true;
    pendingDecision_ = decision;
    return true;
}

MediaImportAgentOutcome PinnedMediaImportAgentExecutor::execute(
    const BackendAgentCommandAssignment& assignment)
{
    // One-shot permit: even direct repeated calls never run the same handler.
    auto snapshot = std::move(pinned_);
    const auto approvedSource = std::move(pendingSource_);
    const MediaImportDecision approvedDecision = pendingDecision_;
    const bool matching = pendingSourceValid_ &&
        snapshot && snapshot->bytesVerified() &&
        assignment.commandId == pendingCommandId_ &&
        assignment.requestFingerprint == pendingFingerprint_ &&
        assignment.backendId == backendId_ &&
        assignment.backendGeneration == generation_ &&
        assignment.deadline > unixNow();
    pendingCommandId_.clear();
    pendingFingerprint_.clear();
    pendingSource_ = {};
    pendingSourceValid_ = false;
    pendingDecision_ = MediaImportDecision::import;
    markerEvidence_.clear();
    if (!matching) return MediaImportAgentOutcome::rejected;
    // Grab the kernel lock BEFORE rechecking installed approval, metadata,
    // and source identity. An exclusive maintainer cannot replace anything
    // until this handler and its descendants have stopped.
    auto runLock = MediaToolsImportRunLock::tryAcquire(
        root_, MediaToolsImportRunLock::Mode::sharedExecution);
    if (!runLock) return MediaImportAgentOutcome::rejected;
    const auto approval = vdrsuite::addons::readRootMediaToolsApproval(root_);
    if (!approval.approved() || approval.sha256 != snapshot->approvedSha256 ||
        approval.version != snapshot->approvedVersion ||
        approval.origin != snapshot->origin ||
        !vdrsuite::addons::installedMediaToolsManifestMatches(
            root_, snapshot->approvedVersion) ||
        // A root-controlled file may have been modified in place since
        // authorization. Do not re-open by name: hash the SAME pinned fd
        // under the child-inherited shared package-maintenance lock.
        !vdrsuite::addons::pinnedMediaToolsHandlerBytesCurrent(
            snapshot->pinnedBinary.get(), snapshot->approvedSha256) ||
        !authorization_.granted(assignment))
        return MediaImportAgentOutcome::rejected;

    std::string sourceId;
    MediaImportDecision requestedDecision = MediaImportDecision::import;
    LocalMediaImportSource local;
    if (!parseMediaImportSelection(
            assignment.payload, sourceId, requestedDecision) ||
        requestedDecision != approvedDecision ||
        !inventory_.resolve(backendId_, sourceId, local) ||
        !safePath(local.sourceRoot) || !safePath(local.videoRoot) ||
        !safePath(local.sourceFile) ||
        local.sourceRoot != approvedSource.sourceRoot ||
        local.videoRoot != approvedSource.videoRoot ||
        local.sourceFile != approvedSource.sourceFile ||
        local.sourceDevice != approvedSource.sourceDevice ||
        local.sourceInode != approvedSource.sourceInode ||
        local.sourceBytes != approvedSource.sourceBytes ||
        local.sourceMtimeSeconds != approvedSource.sourceMtimeSeconds ||
        local.sourceMtimeNanoseconds != approvedSource.sourceMtimeNanoseconds)
        return MediaImportAgentOutcome::rejected;

    if (requestedDecision == MediaImportDecision::desktop ||
        requestedDecision == MediaImportDecision::skip ||
        requestedDecision == MediaImportDecision::duplicate ||
        requestedDecision == MediaImportDecision::release) {
        const auto outcome = markMediaImportSource(
            local, requestedDecision == MediaImportDecision::desktop
                ? MediaImportMarkerDecision::desktop
                : requestedDecision == MediaImportDecision::skip
                    ? MediaImportMarkerDecision::skip
                    : requestedDecision == MediaImportDecision::duplicate
                        ? MediaImportMarkerDecision::duplicate
                        : MediaImportMarkerDecision::release);
        if (outcome == MediaImportMarkerOutcome::marked) {
            markerEvidence_ = mediaImportMarkerEvidence(sourceId, requestedDecision);
            return MediaImportAgentOutcome::marked;
        }
        return outcome == MediaImportMarkerOutcome::rejected
            ? MediaImportAgentOutcome::rejected
            : MediaImportAgentOutcome::unknown;
    }
    return runPinned(snapshot->pinnedBinary.get(), local, assignment.deadline,
                     *runLock, recordingEvidence_,
                     requestedDecision == MediaImportDecision::local);
}

std::string PinnedMediaImportAgentExecutor::takeMarkerEvidence()
{
    auto value = std::move(markerEvidence_);
    markerEvidence_.clear();
    return value;
}

std::string PinnedMediaImportAgentExecutor::takeRecordingEvidence()
{
    auto evidence = std::move(recordingEvidence_);
    recordingEvidence_.clear();
    return evidence;
}

} // namespace vdrsuite::mediatools
