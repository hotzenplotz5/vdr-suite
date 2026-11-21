#include "RecordingMediaSessionRuntime.h"

#include "FfmpegHlsCommandBuilder.h"
#include "MediaSessionRepository.h"
#include "MediaSessionWorkspace.h"

#include <cerrno>
#include <chrono>
#include <memory>
#include <sys/wait.h>
#include <thread>
#include <utility>

namespace
{

constexpr auto HlsReadinessTimeout = std::chrono::seconds(15);
constexpr auto HlsReadinessPollInterval = std::chrono::milliseconds(50);
constexpr auto WorkerShutdownGrace = std::chrono::milliseconds(500);

} // namespace

RecordingMediaSessionProvisionResult RecordingMediaSessionRuntime::provisionHlsAt(
    const std::string& sessionId,
    const std::string& workspaceId,
    const std::string& grantId,
    const MediaPresentationProfile& profile,
    const std::vector<std::string>& sourceSegments,
    int startPositionSeconds,
    const std::vector<double>& segmentDurationsSeconds)
{
    RecordingMediaSessionProvisionResult result;
    if (sessionId.empty() || workspaceId.empty() || grantId.empty() ||
        sourceSegments.empty() || startPositionSeconds <= 0 ||
        segmentDurationsSeconds.size() != sourceSegments.size() ||
        !profile.available || profile.protocol != MediaDeliveryProtocol::Hls ||
        (profile.container != MediaContainer::Fmp4 &&
         profile.container != MediaContainer::MpegTs)) {
        result.reasonCode = "invalid_hls_resume_provision_request";
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
        workspace->prepare(
            workspaceId,
            sourceSegments,
            segmentDurationsSeconds);
    if (!workspaceResult.ready) {
        result.reasonCode = workspaceResult.reasonCode.empty()
            ? "media_workspace_unavailable"
            : workspaceResult.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const MediaPresentationProfile resolvedProfile = transcodePolicy_.apply(profile);
    const FfmpegHlsCommandPlan command =
        FfmpegHlsCommandBuilder().build(
            resolvedProfile,
            startPositionSeconds);
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
        if (readinessProbe_(workspace->directory(), resolvedProfile.container)) {
            ActiveSession active;
            active.pid = pid;
            active.grantId = grantId;
            active.workspace = std::move(workspace);
            active.durationSeconds = 0;
            active.streamProfile = resolvedProfile;
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
