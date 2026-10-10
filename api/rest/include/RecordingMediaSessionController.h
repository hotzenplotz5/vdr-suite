#pragma once

#include "DashboardController.h"
#include "MediaCapabilities.h"
#include "VdrRecording.h"

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

    // Only after live Device authentication, grant verification, and
    // canonical public-ID admission. No native identity reaches the response.
    ApiResponse createDeviceSession(
        const std::string& body,
        const std::string& actorId,
        const VdrRecording& authorizedRecording,
        const std::string& publicRecordingId,
        const std::string& externalMediaPrefix,
        const std::string& cookiePrefix,
        std::string& activatedSessionId) const;

    std::size_t reapInactiveSessions(int idleTimeoutSeconds) const;

private:
    ApiResponse createSessionInternal(
        const std::string& body,
        const std::string& actorId,
        const VdrRecording* authorizedRecording,
        const std::string& publicRecordingId,
        const std::string& externalMediaPrefix,
        const std::string& cookiePrefix,
        std::string* activatedSessionId) const;

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

    ApiResponse selectSubtitleTrack(
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
    mutable std::mutex selectedAudioStreamMutex_;
    mutable std::map<std::string, int> selectedAudioStreamIndexes_;
    mutable std::mutex selectedSubtitleStreamMutex_;
    mutable std::map<std::string, int> selectedSubtitleStreamIndexes_;
    std::string workspaceRoot_;
};
