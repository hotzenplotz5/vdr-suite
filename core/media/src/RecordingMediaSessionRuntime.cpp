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
    : RecordingMediaSessionRuntime(
          repository,
          std::move(workspaceRoot),
          std::move(workerSpawner),
          std::move(workerTerminator),
          std::move(readinessProbe),
          MediaTranscodePolicy::fromEnvironment())
{
}

RecordingMediaSessionRuntime::RecordingMediaSessionRuntime(
    MediaSessionRepository& repository,
    std::string workspaceRoot,
    RecordingDirectSourceRegistry& directSourceRegistry)
    : RecordingMediaSessionRuntime(
          repository,
          std::move(workspaceRoot))
{
    directSourceRegistry_ = &directSourceRegistry;
}

RecordingMediaSessionRuntime::RecordingMediaSessionRuntime(
    MediaSessionRepository& repository,
    std::string workspaceRoot,
    WorkerSpawner workerSpawner,
    WorkerTerminator workerTerminator,
    ReadinessProbe readinessProbe,
    MediaTranscodePolicy transcodePolicy)
    : repository_(repository),
      workspaceRoot_(std::move(workspaceRoot)),
      workerSpawner_(std::move(workerSpawner)),
      workerTerminator_(std::move(workerTerminator)),
      readinessProbe_(std::move(readinessProbe)),
      transcodePolicy_(std::move(transcodePolicy))
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
    const std::string& grantId,
    const MediaPresentationProfile& profile,
    const std::vector<std::string>& sourceSegments)
{
    RecordingMediaSessionProvisionResult result;
    if (sessionId.empty() || workspaceId.empty() || grantId.empty() ||
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

    // Resolve server performance policy exactly once before the worker starts.
    // The selected encoder/preset remains stable for this MediaSession.
    const MediaPresentationProfile resolvedProfile = transcodePolicy_.apply(profile);
    const FfmpegHlsCommandPlan command =
        FfmpegHlsCommandBuilder().build(resolvedProfile);
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

RecordingMediaSessionProvisionResult RecordingMediaSessionRuntime::provisionDirect(
    const std::string& sessionId,
    const std::string& grantId,
    const MediaPresentationProfile& profile,
    const RecordingDirectSourceRegistration& registration)
{
    RecordingMediaSessionProvisionResult result;
    if (directSourceRegistry_ == nullptr || sessionId.empty() || grantId.empty() ||
        !profile.available || profile.protocol != MediaDeliveryProtocol::Progressive ||
        profile.container != MediaContainer::MpegTs ||
        profile.adaptationClass != MediaAdaptationClass::PassThrough ||
        registration.readableBytes == 0) {
        result.reasonCode = "invalid_direct_provision_request";
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_.find(sessionId) != active_.end()) {
            result.reasonCode = "media_session_already_owned";
            return result;
        }
    }

    std::string registrationFailure;
    if (!directSourceRegistry_->registerCompleted(
            sessionId,
            registration,
            registrationFailure)) {
        result.reasonCode = registrationFailure.empty()
            ? "recording_direct_source_unavailable"
            : registrationFailure;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    ActiveSession active;
    active.grantId = grantId;
    active.direct = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto inserted = active_.emplace(sessionId, std::move(active));
        if (!inserted.second) {
            directSourceRegistry_->remove(sessionId);
            repository_.failBundle(sessionId, "media_session_already_owned");
            result.reasonCode = "media_session_already_owned";
            return result;
        }
    }

    if (!repository_.activateBundle(sessionId)) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_.erase(sessionId);
        }
        directSourceRegistry_->remove(sessionId);
        repository_.failBundle(sessionId, "media_session_activation_failed");
        result.reasonCode = "media_session_activation_failed";
        return result;
    }

    result.ready = true;
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
    if (active.direct && directSourceRegistry_ != nullptr) {
        directSourceRegistry_->remove(sessionId);
    }

    const bool bundleEnded = repository_.endBundle(sessionId, reasonCode);
    return workerStopped && bundleEnded;
}

std::size_t RecordingMediaSessionRuntime::reapInactive(int idleTimeoutSeconds)
{
    if (idleTimeoutSeconds < 0 || idleTimeoutSeconds > 86400) {
        return 0;
    }

    std::vector<std::pair<std::string, std::string>> candidates;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        candidates.reserve(active_.size());
        for (const auto& entry : active_) {
            candidates.emplace_back(entry.first, entry.second.grantId);
        }
    }

    std::size_t reaped = 0;
    for (const auto& candidate : candidates) {
        const auto grant = repository_.findResolvedGrant(
            candidate.second,
            idleTimeoutSeconds);
        if (!grant.has_value() || grant->sessionId != candidate.first) {
            continue;
        }

        std::string reasonCode;
        if (!grant->active || grant->revoked) {
            reasonCode = "media_access_revoked";
        }
        else if (grant->expired) {
            reasonCode = "media_access_expired";
        }
        else if (grant->idleExpired) {
            reasonCode = "media_access_idle_expired";
        }

        if (!reasonCode.empty() && stop(candidate.first, reasonCode)) {
            ++reaped;
        }
    }

    return reaped;
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
        if (entry.second.direct && directSourceRegistry_ != nullptr) {
            directSourceRegistry_->remove(entry.first);
        }
        repository_.endBundle(entry.first, "daemon_shutdown");
    }
}
