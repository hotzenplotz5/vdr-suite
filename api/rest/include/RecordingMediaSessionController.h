#pragma once

#include "DashboardController.h"
#include "MediaCapabilities.h"

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class MediaSessionIssuanceService;
class MediaSessionRepository;
class RecordingDirectSourceRegistry;
class RecordingMediaSessionRuntime;
class VdrRecordingIndexUpdater;
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

    struct PendingIndexContext
    {
        std::string backendId;
        std::string recordingId;
        std::string recordingDirectory;
        std::vector<std::string> sourceSegments;
    };

    ApiResponse stopSession(
        const std::string& body,
        const std::string& actorId) const;

    ApiResponse seekSession(
        const std::string& body,
        const std::string& actorId) const;

    ApiResponse playbackStatus(
        const std::string& body,
        const std::string& actorId) const;

    ApiResponse trackStatus(
        const std::string& body,
        const std::string& actorId) const;

    ApiResponse selectAudioTrack(
        const std::string& body,
        const std::string& actorId) const;

    VdrRecordingQueryService& recordingQueryService_;
    MediaSessionRepository& mediaSessionRepository_;
    MediaSessionIssuanceService& mediaSessionIssuanceService_;
    std::unique_ptr<RecordingDirectSourceRegistry> ownedDirectSourceRegistry_;
    RecordingDirectSourceRegistry* directSourceRegistry_ = nullptr;
    std::unique_ptr<RecordingMediaSessionRuntime> mediaSessionRuntime_;
    std::unique_ptr<VdrRecordingIndexUpdater> indexUpdater_;
    mutable std::mutex descriptorCacheMutex_;
    mutable std::map<std::string, CachedSourceDescriptor> descriptorCache_;
    mutable std::mutex pendingIndexMutex_;
    mutable std::map<std::string, PendingIndexContext> pendingIndex_;
    std::string workspaceRoot_;
};
