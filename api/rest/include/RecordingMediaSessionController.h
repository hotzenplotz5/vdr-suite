#pragma once

#include "DashboardController.h"

#include <memory>
#include <string>

class MediaSessionIssuanceService;
class MediaSessionRepository;
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
    ~RecordingMediaSessionController();

    ApiResponse handleRequest(
        const std::string& body,
        const std::string& actorId) const;

    ApiResponse createSession(
        const std::string& body,
        const std::string& actorId) const;

private:
    ApiResponse stopSession(
        const std::string& body,
        const std::string& actorId) const;

    VdrRecordingQueryService& recordingQueryService_;
    MediaSessionRepository& mediaSessionRepository_;
    MediaSessionIssuanceService& mediaSessionIssuanceService_;
    std::unique_ptr<RecordingMediaSessionRuntime> mediaSessionRuntime_;
    std::string workspaceRoot_;
};