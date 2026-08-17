#include "RecordingMediaSessionRuntime.h"

#include "FfmpegHlsCommandBuilder.h"
#include "MediaProcessRunner.h"
#include "MediaSessionRepository.h"
#include "MediaSessionWorkspace.h"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <sys/wait.h>
#include <thread>
#include <utility>

namespace
{

constexpr auto HlsReadinessTimeout = std::chrono::seconds(15);
constexpr auto HlsReadinessPollInterval = std::chrono::milliseconds(50);
constexpr auto WorkerShutdownGrace = std::chrono::milliseconds(500);

bool nonEmptyFile(const std::filesystem::path& path)
{
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) {
        return false;
    }
    const auto size = std::filesystem::file_size(path, error);
    return !error && size > 0;
}

} // namespace

RecordingMediaSessionRuntime::RecordingMediaSessionRuntime(
    MediaSessionRepository& repository,
    std::string workspaceRoot,
    WorkerSpawner workerSpawner,
    WorkerTerminator workerTerminator,
    ReadinessProbe readinessProbe)
    : repository_(repository),
      workspaceRoot_(std::move(workspaceRoot)),
      workerSpawner_(std::move(workerSpawner)),
      workerTerminator_(std::move(workerTerminator)),
      readinessProbe_(std::move(readinessProbe))
{
    if (!workerSpawner_) {
        workerSpawner_ = [](const std::vector<std::string>& argv,
                            const std::string& workingDirectory,
                            const std::string& logPath) {
            return MediaProcessRunner().spawnLogged(argv, workingDirectory, logPath);
        };
    }
    if (!workerTerminator_) {
        workerTerminator_ = [](pid_t pid, std::chrono::milliseconds gracePeriod) {
            return MediaProcessRunner().terminateAndWait(pid, gracePeriod);
        };
    }
    if (!readinessProbe_) {
        readinessProbe_ = &RecordingMediaSessionRuntime::defaultReady;
    }
}

RecordingMediaSessionRuntime::~RecordingMediaSessionRuntime()
{
    stopAll();
}

bool RecordingMediaSessionRuntime::defaultReady(
    const std::string& workspaceDirectory,
    MediaContainer container)
{
    const std::filesystem::path root(workspaceDirectory);
    if (!root.is_absolute() || !nonEmptyFile(root / "master.m3u8")) {
        return false;
    }

    if (container == MediaContainer::Fmp4) {
        return nonEmptyFile(root / "init.mp4") &&
            nonEmptyFile(root / "segment-000000.m4s");
    }
    if (container == MediaContainer::MpegTs) {
        return nonEmptyFile(root / "segment-000000.ts");
    }
    return false;
}

RecordingMediaSessionProvisionResult RecordingMediaSessionRuntime::provisionHls(
    const std::string& sessionId,
    const std::string& workspaceId,
    const MediaPresentationProfile& profile,
    const std::vector<std::string>& sourceSegments)
{
    RecordingMediaSessionProvisionResult result;
    if (sessionId.empty() || workspaceId.empty() ||
        !profile.available || profile.protocol != MediaDeliveryProtocol::Hls ||
        (profile.container != MediaContainer::Fmp4 &&
         profile.container != MediaContainer::MpegTs)) {
        result.reasonCode = "invalid_hls_provision_request";
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_.find(sessionId) != active_.end()) {
            result.reasonCode = "media_session_already_owned";
            return result;
        }
    }

    auto workspace = std::make_unique<MediaSessionWorkspace>(workspaceRoot_);
    const MediaSessionWorkspaceResult workspaceResult =
        workspace->prepare(workspaceId, sourceSegments);
    if (!workspaceResult.ready) {
        result.reasonCode = workspaceResult.reasonCode.empty()
            ? "media_workspace_unavailable"
            : workspaceResult.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const FfmpegHlsCommandPlan command = FfmpegHlsCommandBuilder().build(profile);
    if (!command.valid) {
        result.reasonCode = command.reasonCode.empty()
            ? "media_worker_plan_invalid"
            : command.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const pid_t pid = workerSpawner_(
        command.argv,
        workspace->directory(),
        workspace->logPath());
    if (pid <= 0) {
        result.reasonCode = "media_worker_start_failed";
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const auto deadline = std::chrono::steady_clock::now() + HlsReadinessTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (readinessProbe_(workspace->directory(), profile.container)) {
            ActiveSession active;
            active.pid = pid;
            active.workspace = std::move(workspace);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                const auto inserted = active_.emplace(sessionId, std::move(active));
                if (!inserted.second) {
                    workerTerminator_(pid, WorkerShutdownGrace);
                    repository_.failBundle(sessionId, "media_session_already_owned");
                    result.reasonCode = "media_session_already_owned";
                    return result;
                }
            }

            if (!repository_.activateBundle(sessionId)) {
                ActiveSession failed;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    auto found = active_.find(sessionId);
                    if (found != active_.end()) {
                        failed = std::move(found->second);
                        active_.erase(found);
                    }
                }
                if (failed.pid > 0) {
                    workerTerminator_(failed.pid, WorkerShutdownGrace);
                }
                repository_.failBundle(sessionId, "media_session_activation_failed");
                result.reasonCode = "media_session_activation_failed";
                return result;
            }

            result.ready = true;
            return result;
        }

        int status = 0;
        const pid_t waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            result.reasonCode = "media_worker_exited_before_ready";
            repository_.failBundle(sessionId, result.reasonCode);
            return result;
        }
        if (waited < 0 && errno != EINTR) {
            result.reasonCode = "media_worker_wait_failed";
            workerTerminator_(pid, WorkerShutdownGrace);
            repository_.failBundle(sessionId, result.reasonCode);
            return result;
        }

        std::this_thread::sleep_for(HlsReadinessPollInterval);
    }

    workerTerminator_(pid, WorkerShutdownGrace);
    result.reasonCode = "media_hls_not_ready";
    repository_.failBundle(sessionId, result.reasonCode);
    return result;
}

bool RecordingMediaSessionRuntime::stop(
    const std::string& sessionId,
    const std::string& reasonCode)
{
    if (sessionId.empty() || reasonCode.empty()) return false;

    ActiveSession active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = active_.find(sessionId);
        if (found == active_.end()) return false;
        active = std::move(found->second);
        active_.erase(found);
    }

    bool workerStopped = true;
    if (active.pid > 0) {
        workerStopped = workerTerminator_(active.pid, WorkerShutdownGrace);
    }

    const bool bundleEnded = repository_.endBundle(sessionId, reasonCode);
    return workerStopped && bundleEnded;
}

void RecordingMediaSessionRuntime::stopAll()
{
    std::map<std::string, ActiveSession> active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active.swap(active_);
    }

    for (auto& entry : active) {
        if (entry.second.pid > 0) {
            workerTerminator_(entry.second.pid, WorkerShutdownGrace);
        }
        repository_.endBundle(entry.first, "daemon_shutdown");
    }
}