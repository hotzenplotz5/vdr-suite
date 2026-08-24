#include "VdrRecordingIndexUpdater.h"

#include <cerrno>
#include <filesystem>
#include <grp.h>
#include <map>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace
{

constexpr const char* VdrBinary = "/usr/bin/vdr";

bool isSafeTsRecording(
    const std::filesystem::path& directory,
    const std::vector<std::string>& sourceSegments,
    std::filesystem::path& canonicalDirectory,
    struct stat& directoryStatus,
    std::string& reasonCode)
{
    if (!directory.is_absolute() || sourceSegments.empty()) {
        reasonCode = "recording_index_update_invalid_source";
        return false;
    }

    std::error_code error;
    canonicalDirectory = std::filesystem::weakly_canonical(directory, error);
    if (error || !canonicalDirectory.is_absolute() ||
        !std::filesystem::is_directory(canonicalDirectory, error) || error) {
        reasonCode = "recording_index_update_invalid_source";
        return false;
    }

    if (std::filesystem::exists(canonicalDirectory / ".timer", error)) {
        reasonCode = "recording_index_update_recording_active";
        return false;
    }
    if (error) {
        reasonCode = "recording_index_update_source_unavailable";
        return false;
    }

    for (const auto& value : sourceSegments) {
        const std::filesystem::path source(value);
        if (!source.is_absolute() || source.extension() != ".ts") {
            reasonCode = "recording_index_update_not_ts";
            return false;
        }
        const std::filesystem::path canonicalSource =
            std::filesystem::weakly_canonical(source, error);
        if (error || canonicalSource.parent_path() != canonicalDirectory ||
            !std::filesystem::is_regular_file(canonicalSource, error) || error) {
            reasonCode = "recording_index_update_invalid_source";
            return false;
        }
    }

    if (::stat(canonicalDirectory.c_str(), &directoryStatus) != 0) {
        reasonCode = "recording_index_update_source_unavailable";
        return false;
    }

    return true;
}

std::string canonicalKey(const std::string& recordingDirectory)
{
    std::error_code error;
    const std::filesystem::path canonical =
        std::filesystem::weakly_canonical(recordingDirectory, error);
    return error ? std::string{} : canonical.string();
}

} // namespace

struct VdrRecordingIndexUpdater::SharedState
{
    struct Job
    {
        pid_t pid = -1;
        VdrRecordingIndexUpdateResult result;
    };

    mutable std::mutex mutex;
    std::map<std::string, Job> jobs;
};

VdrRecordingIndexUpdater::VdrRecordingIndexUpdater(
    WorkerSpawner workerSpawner,
    WorkerWaiter workerWaiter)
    : state_(std::make_shared<SharedState>()),
      workerSpawner_(workerSpawner ? std::move(workerSpawner) : &spawnAsOwner),
      workerWaiter_(workerWaiter ? std::move(workerWaiter) : &waitForWorker)
{
}

VdrRecordingIndexUpdateResult VdrRecordingIndexUpdater::ensure(
    const std::string& recordingDirectory,
    const std::vector<std::string>& sourceSegments)
{
    std::filesystem::path canonicalDirectory;
    struct stat directoryStatus {};
    std::string reasonCode;
    if (!isSafeTsRecording(
            recordingDirectory,
            sourceSegments,
            canonicalDirectory,
            directoryStatus,
            reasonCode)) {
        VdrRecordingIndexUpdateResult result;
        result.state = VdrRecordingIndexUpdateState::Failed;
        result.reasonCode = std::move(reasonCode);
        return result;
    }

    const std::string key = canonicalDirectory.string();
    std::lock_guard<std::mutex> lock(state_->mutex);
    const auto existing = state_->jobs.find(key);
    if (existing != state_->jobs.end()) {
        return existing->second.result;
    }

    const pid_t pid = workerSpawner_(
        key,
        directoryStatus.st_uid,
        directoryStatus.st_gid);
    if (pid <= 0) {
        VdrRecordingIndexUpdateResult result;
        result.state = VdrRecordingIndexUpdateState::Failed;
        result.reasonCode = "recording_index_update_start_failed";
        state_->jobs.emplace(key, SharedState::Job{-1, result});
        return result;
    }

    VdrRecordingIndexUpdateResult running;
    running.state = VdrRecordingIndexUpdateState::Running;
    state_->jobs.emplace(key, SharedState::Job{pid, running});

    const std::shared_ptr<SharedState> shared = state_;
    const WorkerWaiter waiter = workerWaiter_;
    std::thread([shared, key, pid, waiter]() {
        const int exitCode = waiter(pid);
        std::lock_guard<std::mutex> stateLock(shared->mutex);
        const auto found = shared->jobs.find(key);
        if (found == shared->jobs.end() || found->second.pid != pid) return;
        found->second.pid = -1;
        if (exitCode == 0) {
            found->second.result.state =
                VdrRecordingIndexUpdateState::Succeeded;
            found->second.result.reasonCode.clear();
        }
        else {
            found->second.result.state =
                VdrRecordingIndexUpdateState::Failed;
            found->second.result.reasonCode =
                "recording_index_update_failed";
        }
    }).detach();

    return running;
}

VdrRecordingIndexUpdateResult VdrRecordingIndexUpdater::status(
    const std::string& recordingDirectory) const
{
    const std::string key = canonicalKey(recordingDirectory);
    if (key.empty()) {
        VdrRecordingIndexUpdateResult result;
        result.state = VdrRecordingIndexUpdateState::Failed;
        result.reasonCode = "recording_index_update_invalid_source";
        return result;
    }

    std::lock_guard<std::mutex> lock(state_->mutex);
    const auto found = state_->jobs.find(key);
    if (found == state_->jobs.end()) {
        return {};
    }
    return found->second.result;
}

pid_t VdrRecordingIndexUpdater::spawnAsOwner(
    const std::string& recordingDirectory,
    uid_t ownerUid,
    gid_t ownerGid)
{
    if (::access(VdrBinary, X_OK) != 0) return -1;

    const pid_t pid = ::fork();
    if (pid != 0) return pid;

    if (::geteuid() == 0) {
        if (::setgroups(0, nullptr) != 0 ||
            ::setgid(ownerGid) != 0 ||
            ::setuid(ownerUid) != 0) {
            ::_exit(126);
        }
    }
    else if (::geteuid() != ownerUid) {
        ::_exit(126);
    }

    const std::string argument =
        "--updindex=" + recordingDirectory;
    ::execl(
        VdrBinary,
        VdrBinary,
        argument.c_str(),
        static_cast<char*>(nullptr));
    ::_exit(127);
}

int VdrRecordingIndexUpdater::waitForWorker(pid_t pid)
{
    if (pid <= 0) return 255;
    int status = 0;
    pid_t waited = -1;
    do {
        waited = ::waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);

    if (waited != pid || !WIFEXITED(status)) return 255;
    return WEXITSTATUS(status);
}
