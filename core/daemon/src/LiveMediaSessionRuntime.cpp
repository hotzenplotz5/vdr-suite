#include "LiveMediaSessionRuntime.h"

#include "FfmpegHlsCommandBuilder.h"
#include "FfprobeLiveSource.h"
#include "MediaPresentationSelector.h"
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
constexpr auto ProbeTimeout = std::chrono::seconds(8);
constexpr std::size_t ProbeOutputLimit = 64U * 1024U;
constexpr auto HlsReadinessTimeout = std::chrono::seconds(15);
constexpr auto HlsReadinessPollInterval = std::chrono::milliseconds(100);
constexpr auto WorkerShutdownGrace = std::chrono::milliseconds(500);

bool nonEmptyFile(const std::filesystem::path& path)
{
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) return false;
    const auto size = std::filesystem::file_size(path, error);
    return !error && size > 0;
}
}

LiveMediaSessionRuntime::LiveMediaSessionRuntime(
    MediaSessionRepository& repository,
    vdrsuite::agent::BackendAgentLiveProviderRuntime& providerRuntime,
    std::string workspaceRoot,
    ProbeRunner probeRunner,
    WorkerSpawner workerSpawner,
    WorkerTerminator workerTerminator,
    ReadinessProbe readinessProbe,
    MediaTranscodePolicy transcodePolicy)
    : repository_(repository),
      providerRuntime_(providerRuntime),
      workspaceRoot_(std::move(workspaceRoot)),
      probeRunner_(std::move(probeRunner)),
      workerSpawner_(std::move(workerSpawner)),
      workerTerminator_(std::move(workerTerminator)),
      readinessProbe_(std::move(readinessProbe)),
      transcodePolicy_(std::move(transcodePolicy))
{
    if (!probeRunner_) {
        probeRunner_ = [](const std::vector<std::string>& argv,
                          const std::string& workingDirectory,
                          std::chrono::milliseconds timeout,
                          std::size_t maximumOutputBytes) {
            return MediaProcessRunner().runAndCapture(
                argv, workingDirectory, timeout, maximumOutputBytes);
        };
    }
    if (!workerSpawner_) {
        workerSpawner_ = [](const std::vector<std::string>& argv,
                            const std::string& workingDirectory,
                            const std::string& logPath) {
            return MediaProcessRunner().spawnLogged(argv, workingDirectory, logPath);
        };
    }
    if (!workerTerminator_) {
        workerTerminator_ = [](pid_t pid, std::chrono::milliseconds grace) {
            return MediaProcessRunner().terminateAndWait(pid, grace);
        };
    }
    if (!readinessProbe_) readinessProbe_ = &LiveMediaSessionRuntime::defaultReady;
}

LiveMediaSessionRuntime::~LiveMediaSessionRuntime()
{
    stopAll();
}

bool LiveMediaSessionRuntime::defaultReady(
    const std::string& workspaceDirectory,
    MediaContainer container)
{
    const std::filesystem::path root(workspaceDirectory);
    if (!root.is_absolute() || !nonEmptyFile(root / "master.m3u8")) return false;
    if (container == MediaContainer::Fmp4)
        return nonEmptyFile(root / "init.mp4") &&
            nonEmptyFile(root / "segment-000000.m4s");
    if (container == MediaContainer::MpegTs)
        return nonEmptyFile(root / "segment-000000.ts");
    return false;
}

LiveMediaSessionProvisionResult LiveMediaSessionRuntime::provisionHls(
    const std::string& sessionId,
    const std::string& workspaceId,
    const std::string& leaseId,
    const std::string& grantId,
    const vdrsuite::agent::BackendAgentLiveProviderPreparation& preparation,
    const ClientMediaCapabilities& clientCapabilities)
{
    LiveMediaSessionProvisionResult result;
    if (sessionId.empty() || workspaceId.empty() || leaseId.empty() || grantId.empty() ||
        !preparation.valid || !preparation.pin.valid) {
        result.reasonCode = "invalid_live_hls_provision_request";
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
    const auto workspaceResult = workspace->prepareLive(workspaceId);
    if (!workspaceResult.ready) {
        result.reasonCode = workspaceResult.reasonCode.empty()
            ? "media_workspace_unavailable" : workspaceResult.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const auto opened = providerRuntime_.open(preparation, leaseId);
    if (!opened.opened) {
        result.reasonCode = opened.reasonCode.empty()
            ? "live_provider_open_failed" : opened.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }
    const auto closeProvider = [&]() {
        std::string ignored;
        providerRuntime_.close(preparation, leaseId, ignored);
    };

    FfprobeLiveSource probe;
    const auto probePlan = probe.commandPlan(opened.unixSocketPath);
    if (!probePlan.valid) {
        closeProvider();
        result.reasonCode = probePlan.reasonCode.empty()
            ? "live_probe_plan_invalid" : probePlan.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }
    const auto probeCapture = probeRunner_(
        probePlan.argv, workspace->directory(), ProbeTimeout, ProbeOutputLimit);
    if (!probeCapture.success) {
        closeProvider();
        result.reasonCode = probeCapture.reasonCode.empty()
            ? "live_probe_failed" : probeCapture.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }
    const auto parsed = probe.parse(probeCapture.output);
    if (!parsed.valid) {
        closeProvider();
        result.reasonCode = parsed.reasonCode.empty()
            ? "live_probe_payload_invalid" : parsed.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }
    result.source = parsed.source;

    MediaPresentationSelector selector;
    MediaPresentationProfile profile = selector.select(result.source, clientCapabilities);
    if (!profile.available || profile.protocol != MediaDeliveryProtocol::Hls) {
        closeProvider();
        result.reasonCode = profile.reason.empty()
            ? "live_presentation_unavailable" : profile.reason;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }
    profile = transcodePolicy_.apply(profile);
    if (!profile.available) {
        closeProvider();
        result.reasonCode = profile.reason.empty()
            ? "live_transcode_capacity_unproven" : profile.reason;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }
    result.presentation = profile;

    std::string providerReason;
    if (!providerRuntime_.current(preparation, providerReason)) {
        closeProvider();
        result.reasonCode = providerReason.empty()
            ? "live_provider_fence_stale" : providerReason;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const auto command = FfmpegHlsCommandBuilder().buildLive(
        profile, opened.unixSocketPath);
    if (!command.valid) {
        closeProvider();
        result.reasonCode = command.reasonCode.empty()
            ? "live_worker_plan_invalid" : command.reasonCode;
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }
    const pid_t pid = workerSpawner_(command.argv, workspace->directory(), workspace->logPath());
    if (pid <= 0) {
        closeProvider();
        result.reasonCode = "live_worker_start_failed";
        repository_.failBundle(sessionId, result.reasonCode);
        return result;
    }

    const auto failRunning = [&](const std::string& reason, bool workerExited) {
        if (!workerExited) workerTerminator_(pid, WorkerShutdownGrace);
        closeProvider();
        repository_.failBundle(sessionId, reason);
    };

    const auto deadline = std::chrono::steady_clock::now() + HlsReadinessTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        int status = 0;
        const pid_t waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            result.reasonCode = "live_worker_exited_before_ready";
            failRunning(result.reasonCode, true);
            return result;
        }
        if (waited < 0 && errno != EINTR) {
            result.reasonCode = "live_worker_wait_failed";
            failRunning(result.reasonCode, false);
            return result;
        }

        const auto providerStatus = providerRuntime_.status(preparation, leaseId);
        if (!providerStatus.current) {
            result.reasonCode = providerStatus.reasonCode.empty()
                ? "live_provider_terminal_before_ready" : providerStatus.reasonCode;
            failRunning(result.reasonCode, false);
            return result;
        }

        if (readinessProbe_(workspace->directory(), profile.container)) {
            ActiveSession active;
            active.pid = pid;
            active.leaseId = leaseId;
            active.grantId = grantId;
            active.preparation = preparation;
            active.workspace = std::move(workspace);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                const auto inserted = active_.emplace(sessionId, std::move(active));
                if (!inserted.second) {
                    failRunning("media_session_already_owned", false);
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
                finishTaken(sessionId, std::move(failed),
                    "media_session_activation_failed", false);
                result.reasonCode = "media_session_activation_failed";
                return result;
            }
            result.ready = true;
            result.workerPid = pid;
            result.reasonCode = "live_media_session_active";
            return result;
        }
        std::this_thread::sleep_for(HlsReadinessPollInterval);
    }

    result.reasonCode = "live_hls_not_ready";
    failRunning(result.reasonCode, false);
    return result;
}

bool LiveMediaSessionRuntime::finishTaken(
    const std::string& sessionId,
    ActiveSession&& active,
    const std::string& reasonCode,
    bool workerAlreadyExited)
{
    bool workerStopped = true;
    if (!workerAlreadyExited && active.pid > 0)
        workerStopped = workerTerminator_(active.pid, WorkerShutdownGrace);
    std::string providerReason;
    const bool providerClosed = providerRuntime_.close(
        active.preparation, active.leaseId, providerReason);
    const bool bundleEnded = repository_.endBundle(sessionId, reasonCode);
    return workerStopped && providerClosed && bundleEnded;
}

bool LiveMediaSessionRuntime::stop(
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
    return finishTaken(sessionId, std::move(active), reasonCode, false);
}

std::size_t LiveMediaSessionRuntime::reapInactive(int idleTimeoutSeconds)
{
    if (idleTimeoutSeconds < 0 || idleTimeoutSeconds > 86400) return 0;
    struct Candidate { std::string sessionId; std::string grantId; };
    std::vector<Candidate> candidates;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        candidates.reserve(active_.size());
        for (const auto& entry : active_)
            candidates.push_back({entry.first, entry.second.grantId});
    }

    std::size_t reaped = 0;
    for (const auto& candidate : candidates) {
        std::string reasonCode;
        bool workerExited = false;
        vdrsuite::agent::BackendAgentLiveProviderPreparation preparation;
        std::string leaseId;
        pid_t pid = -1;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto found = active_.find(candidate.sessionId);
            if (found == active_.end()) continue;
            preparation = found->second.preparation;
            leaseId = found->second.leaseId;
            pid = found->second.pid;
        }
        int status = 0;
        const pid_t waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            reasonCode = "live_worker_exited";
            workerExited = true;
        }
        else if (waited < 0 && errno != EINTR) {
            reasonCode = "live_worker_wait_failed";
        }
        if (reasonCode.empty()) {
            const auto providerStatus = providerRuntime_.status(preparation, leaseId);
            if (!providerStatus.current)
                reasonCode = providerStatus.reasonCode.empty()
                    ? "live_provider_terminal" : providerStatus.reasonCode;
        }

        if (reasonCode.empty()) {
            const auto grant = repository_.findResolvedGrant(
                candidate.grantId, idleTimeoutSeconds);
            if (!grant.has_value() || grant->sessionId != candidate.sessionId) {
                reasonCode = "media_access_grant_missing";
            }
            else if (!grant->active || grant->revoked) reasonCode = "media_access_revoked";
            else if (grant->expired) reasonCode = "media_access_expired";
            else if (grant->idleExpired) reasonCode = "media_access_idle_expired";
        }
        if (reasonCode.empty()) continue;

        ActiveSession active;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto found = active_.find(candidate.sessionId);
            if (found == active_.end()) continue;
            active = std::move(found->second);
            active_.erase(found);
        }
        if (finishTaken(candidate.sessionId, std::move(active), reasonCode, workerExited))
            ++reaped;
    }
    return reaped;
}

void LiveMediaSessionRuntime::stopAll()
{
    std::map<std::string, ActiveSession> active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active.swap(active_);
    }
    for (auto& entry : active)
        finishTaken(entry.first, std::move(entry.second), "daemon_shutdown", false);
}

std::size_t LiveMediaSessionRuntime::activeCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return active_.size();
}

pid_t LiveMediaSessionRuntime::workerPid(const std::string& sessionId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = active_.find(sessionId);
    return found == active_.end() ? -1 : found->second.pid;
}
