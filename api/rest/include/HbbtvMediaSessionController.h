#pragma once

#include "HbbtvApiRuntime.h"

#include <cstddef>
#include <memory>
#include <string>

class HbbtvMediaSessionRuntime;
class MediaSessionIssuanceService;
class MediaSessionRepository;

class HbbtvMediaSessionController
{
public:
    HbbtvMediaSessionController(
        MediaSessionRepository& mediaSessionRepository,
        MediaSessionIssuanceService& mediaSessionIssuanceService,
        std::string workspaceRoot);
    ~HbbtvMediaSessionController();

    ApiResponse handleMutation(
        const HbbtvMediaSessionMutationRequest& request) const;
    std::size_t reapInactiveSessions(
        int idleTimeoutSeconds) const;

private:
    ApiResponse start(
        const HbbtvMediaSessionMutationRequest& request) const;
    ApiResponse stop(
        const HbbtvMediaSessionMutationRequest& request,
        const std::string& reasonCode) const;

    MediaSessionRepository& mediaSessionRepository_;
    MediaSessionIssuanceService& mediaSessionIssuanceService_;
    std::unique_ptr<HbbtvMediaSessionRuntime> runtime_;
};
