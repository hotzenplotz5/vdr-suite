#pragma once

#include "MediaCapabilities.h"

#include <chrono>
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
    ~RecordingMediaSessionRuntime();

    RecordingMediaSessionRuntime(const RecordingMediaSessionRuntime&) = delete;
    RecordingMediaSessionRuntime& operator=(const RecordingMediaSessionRuntime&) = delete;

    RecordingMediaSessionProvisionResult provisionHls(
        const std::string& sessionId,
        const std::string& workspaceId,
        const MediaPresentationProfile& profile,
        const std::vector<std::string>& sourceSegments);

    bool stop(
        const std::string& sessionId,
        const std::string& reasonCode);

    void stopAll();

private:
    struct ActiveSession
    {
        pid_t pid = -1;
        std::unique_ptr<MediaSessionWorkspace> workspace;
    };

    static bool defaultReady(
        const std::string& workspaceDirectory,
        MediaContainer container);

    MediaSessionRepository& repository_;
    std::string workspaceRoot_;
    WorkerSpawner workerSpawner_;
    WorkerTerminator workerTerminator_;
    ReadinessProbe readinessProbe_;
    std::mutex mutex_;
    std::map<std::string, ActiveSession> active_;
};