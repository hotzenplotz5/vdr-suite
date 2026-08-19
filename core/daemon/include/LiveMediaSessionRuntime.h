#pragma once

#include "BackendAgentLiveProviderRuntime.h"
#include "MediaCapabilities.h"
#include "MediaProcessRunner.h"
#include "MediaTranscodePolicy.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

class MediaSessionRepository;
class MediaSessionWorkspace;

struct LiveMediaSessionProvisionResult
{
    bool ready = false;
    std::string reasonCode;
    MediaSourceDescriptor source;
    MediaPresentationProfile presentation;
    pid_t workerPid = -1;
};

class LiveMediaSessionRuntime
{
public:
    using ProbeRunner = std::function<MediaProcessCaptureResult(
        const std::vector<std::string>&,
        const std::string&,
        std::chrono::milliseconds,
        std::size_t)>;
    using WorkerSpawner = std::function<pid_t(
        const std::vector<std::string>&,
        const std::string&,
        const std::string&)>;
    using WorkerTerminator = std::function<bool(
        pid_t,
        std::chrono::milliseconds)>;
    using ReadinessProbe = std::function<bool(
        const std::string&,
        MediaContainer)>;

    LiveMediaSessionRuntime(
        MediaSessionRepository& repository,
        vdrsuite::agent::BackendAgentLiveProviderRuntime& providerRuntime,
        std::string workspaceRoot,
        ProbeRunner probeRunner = {},
        WorkerSpawner workerSpawner = {},
        WorkerTerminator workerTerminator = {},
        ReadinessProbe readinessProbe = {},
        MediaTranscodePolicy transcodePolicy = MediaTranscodePolicy::fromEnvironment());
    ~LiveMediaSessionRuntime();

    // Primary Phase-65.B hot path. One conditioned native TS consumer is
    // remuxed directly to a live fragmented-MP4 FIFO; there is no ffprobe or
    // HLS readiness barrier before the browser can connect.
    LiveMediaSessionProvisionResult provisionStream(
        const std::string& sessionId,
        const std::string& workspaceId,
        const std::string& leaseId,
        const std::string& grantId,
        const vdrsuite::agent::BackendAgentLiveProviderPreparation& preparation,
        const ClientMediaCapabilities& clientCapabilities);

    // Compatibility entry point for branch-local callers while 65.B is being
    // closed out. It provisions the same direct stream and must not re-enable
    // the old probe/HLS hot path.
    LiveMediaSessionProvisionResult provisionHls(
        const std::string& sessionId,
        const std::string& workspaceId,
        const std::string& leaseId,
        const std::string& grantId,
        const vdrsuite::agent::BackendAgentLiveProviderPreparation& preparation,
        const ClientMediaCapabilities& clientCapabilities);

    bool stop(
        const std::string& sessionId,
        const std::string& reasonCode);

    std::size_t reapInactive(int idleTimeoutSeconds);
    void stopAll();

    std::size_t activeCount() const;
    pid_t workerPid(const std::string& sessionId) const;

private:
    struct ActiveSession
    {
        pid_t pid = -1;
        std::string leaseId;
        std::string grantId;
        vdrsuite::agent::BackendAgentLiveProviderPreparation preparation;
        std::unique_ptr<MediaSessionWorkspace> workspace;
    };

    static bool defaultReady(
        const std::string& workspaceDirectory,
        MediaContainer container);

    bool finishTaken(
        const std::string& sessionId,
        ActiveSession&& active,
        const std::string& reasonCode,
        bool workerAlreadyExited);

    MediaSessionRepository& repository_;
    vdrsuite::agent::BackendAgentLiveProviderRuntime& providerRuntime_;
    std::string workspaceRoot_;
    ProbeRunner probeRunner_;
    WorkerSpawner workerSpawner_;
    WorkerTerminator workerTerminator_;
    ReadinessProbe readinessProbe_;
    MediaTranscodePolicy transcodePolicy_;
    mutable std::mutex mutex_;
    std::map<std::string, ActiveSession> active_;
};
