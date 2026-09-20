#pragma once

#include "MediaCapabilities.h"
#include "MediaProcessRunner.h"
#include "MediaTranscodePolicy.h"
#include "SuiteBridgeHbbtvMediaResolver.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>

class MediaSessionRepository;
class MediaSessionWorkspace;

struct HbbtvMediaSessionProvisionResult
{
    bool ready = false;
    std::string reasonCode;
    MediaPresentationProfile presentation;
    pid_t workerPid = -1;
};

struct HbbtvMediaSessionActive
{
    std::string mediaSessionId;
    std::uint64_t mediaRevision = 0;
};

class HbbtvMediaSessionRuntime
{
public:
    using WorkerSpawner = std::function<pid_t(
        const std::vector<std::string>&,
        const std::string&,
        const std::string&)>;
    using WorkerTerminator = std::function<bool(
        pid_t,
        std::chrono::milliseconds)>;

    HbbtvMediaSessionRuntime(
        MediaSessionRepository& repository,
        std::string workspaceRoot,
        WorkerSpawner workerSpawner = {},
        WorkerTerminator workerTerminator = {},
        MediaTranscodePolicy transcodePolicy =
            MediaTranscodePolicy::fromEnvironment());
    ~HbbtvMediaSessionRuntime();

    HbbtvMediaSessionProvisionResult provisionStream(
        const std::string& mediaSessionId,
        const std::string& workspaceId,
        const std::string& grantId,
        const std::string& hbbtvSessionId,
        const std::string& backendId,
        const HbbtvMediaSource& media,
        const ClientMediaCapabilities& clientCapabilities);

    bool stop(
        const std::string& mediaSessionId,
        const std::string& reasonCode);
    bool stopForApplicationSession(
        const std::string& hbbtvSessionId,
        const std::string& reasonCode);
    std::size_t reapInactive(int idleTimeoutSeconds);
    void stopAll();

    std::optional<HbbtvMediaSessionActive>
    activeForApplicationSession(
        const std::string& hbbtvSessionId) const;
    std::size_t activeCount() const;

private:
    struct ActiveSession
    {
        pid_t pid = -1;
        std::string grantId;
        std::string hbbtvSessionId;
        std::uint64_t mediaRevision = 0;
        std::unique_ptr<MediaSessionWorkspace> workspace;
    };

    bool finishTaken(
        const std::string& mediaSessionId,
        ActiveSession&& active,
        const std::string& reasonCode,
        bool workerAlreadyExited);

    MediaSessionRepository& repository_;
    std::string workspaceRoot_;
    WorkerSpawner workerSpawner_;
    WorkerTerminator workerTerminator_;
    MediaTranscodePolicy transcodePolicy_;
    mutable std::mutex mutex_;
    std::map<std::string, ActiveSession> active_;
};
