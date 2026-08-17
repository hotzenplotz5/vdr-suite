#pragma once

#include "DashboardController.h"

#include <string>

class MediaSessionIssuanceService;
class MediaSessionRepository;
class VdrRecordingQueryService;

class RecordingMediaSessionController
{
public:
    RecordingMediaSessionController(
        VdrRecordingQueryService& recordingQueryService,
        MediaSessionRepository& mediaSessionRepository,
        MediaSessionIssuanceService& mediaSessionIssuanceService,
        std::string workspaceRoot);

    ApiResponse createSession(
        const std::string& body,
        const std::string& actorId) const;

private:
    VdrRecordingQueryService& recordingQueryService_;
    MediaSessionRepository& mediaSessionRepository_;
    MediaSessionIssuanceService& mediaSessionIssuanceService_;
    std::string workspaceRoot_;
};
