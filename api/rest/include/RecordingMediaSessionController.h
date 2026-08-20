#pragma once

#include "DashboardController.h"
#include "MediaCapabilities.h"

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>

class MediaSessionIssuanceService;
class MediaSessionRepository;
class RecordingDirectSourceRegistry;
class RecordingMediaSessionRuntime;
class VdrRecordingQueryService;

class RecordingMediaSessionController
{
public:
    RecordingMediaSessionController(
        VdrRecordingQueryService& recordingQueryService,
        MediaSessionRepository& mediaSessionRepository,
        MediaSessionIssuanceService& mediaSessionIssuanceService,
        std::string workspaceRoot);

    RecordingMediaSessionController(
        VdrRecordingQueryService& recordingQueryService,
        MediaSessionRepository& mediaSessionRepository,
        MediaSessionIssuanceService& mediaSessionIssuanceService,
        RecordingDirectSourceRegistry& directSourceRegistry,
        std::string workspaceRoot);

    ~RecordingMediaSessionController();

    ApiResponse handleRequest(
        const std::string& body,
        const std::string& actorId) const;

    ApiResponse createSession(
        const std::string& body,
        const std::string& actorId) const;

    std::size_t reapInactiveSessions(int idleTimeoutSeconds) const;

private:
    struct CachedSourceDescriptor
    {
        std::string sourceFingerprint;
        MediaSourceDescriptor source;
    };

    ApiResponse stopSession(
        const std::string& body,
        const std::string& actorId) const;

    VdrRecordingQueryService& recordingQueryService_;
    MediaSessionRepository& mediaSessionRepository_;
    MediaSessionIssuanceService& mediaSessionIssuanceService_;
    std::unique_ptr<RecordingDirectSourceRegistry> ownedDirectSourceRegistry_;
    RecordingDirectSourceRegistry* directSourceRegistry_ = nullptr;
    std::unique_ptr<RecordingMediaSessionRuntime> mediaSessionRuntime_;
    mutable std::mutex descriptorCacheMutex_;
    mutable std::map<std::string, CachedSourceDescriptor> descriptorCache_;
    std::string workspaceRoot_;
};
