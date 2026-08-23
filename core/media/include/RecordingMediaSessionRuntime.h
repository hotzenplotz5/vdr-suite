#pragma once

#include "MediaCapabilities.h"
#include "MediaTranscodePolicy.h"
#include "RecordingDirectSourceRegistry.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

class MediaSessionRepository;
class MediaSessionWorkspace;

struct RecordingMediaSessionProvisionResult
{
    bool ready = false;
    std::string reasonCode;
};

struct RecordingMediaSessionSeekResult
{
    bool repositioned = false;
    std::string reasonCode;
    int positionSeconds = 0;
    int durationSeconds = 0;
};

class RecordingMediaSessionRuntime
{
public:
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

    RecordingMediaSessionRuntime(
        MediaSessionRepository& repository,
        std::string workspaceRoot,
        WorkerSpawner workerSpawner = {},
        WorkerTerminator workerTerminator = {},
        ReadinessProbe readinessProbe = {});

    RecordingMediaSessionRuntime(
        MediaSessionRepository& repository,
        std::string workspaceRoot,
        RecordingDirectSourceRegistry& directSourceRegistry);

    RecordingMediaSessionRuntime(
        MediaSessionRepository& repository,
        std::string workspaceRoot,
        WorkerSpawner workerSpawner,
        WorkerTerminator workerTerminator,
        ReadinessProbe readinessProbe,
        MediaTranscodePolicy transcodePolicy);

    ~RecordingMediaSessionRuntime();

    RecordingMediaSessionRuntime(const RecordingMediaSessionRuntime&) = delete;
    RecordingMediaSessionRuntime& operator=(const RecordingMediaSessionRuntime&) = delete;

    RecordingMediaSessionProvisionResult provisionHls(
        const std::string& sessionId,
        const std::string& workspaceId,
        const std::string& grantId,
        const MediaPresentationProfile& profile,
        const std::vector<std::string>& sourceSegments);

    RecordingMediaSessionProvisionResult provisionStream(
        const std::string& sessionId,
        const std::string& workspaceId,
        const std::string& grantId,
        const MediaPresentationProfile& profile,
        const std::vector<std::string>& sourceSegments,
        int durationSeconds = 0);

    RecordingMediaSessionProvisionResult provisionDirect(
        const std::string& sessionId,
        const std::string& grantId,
        const MediaPresentationProfile& profile,
        const RecordingDirectSourceRegistration& registration);

    RecordingMediaSessionSeekResult seekStream(
        const std::string& sessionId,
        int positionSeconds);

    bool stop(
        const std::string& sessionId,
        const std::string& reasonCode);

    std::size_t reapInactive(int idleTimeoutSeconds);

    void stopAll();

private:
    struct ActiveSession
    {
        pid_t pid = -1;
        std::string grantId;
        std::unique_ptr<MediaSessionWorkspace> workspace;
        bool direct = false;
        bool continuousStream = false;
        int durationSeconds = 0;
        std::uint64_t streamGeneration = 0;
        MediaPresentationProfile streamProfile;
    };

    static bool defaultReady(
        const std::string& workspaceDirectory,
        MediaContainer container);

    MediaSessionRepository& repository_;
    std::string workspaceRoot_;
    WorkerSpawner workerSpawner_;
    WorkerTerminator workerTerminator_;
    ReadinessProbe readinessProbe_;
    MediaTranscodePolicy transcodePolicy_;
    RecordingDirectSourceRegistry* directSourceRegistry_ = nullptr;
    std::mutex mutex_;
    std::map<std::string, ActiveSession> active_;
};
