#pragma once

#include "DashboardController.h"

#include <cstddef>
#include <memory>
#include <string>

namespace vdrsuite::agent { class BackendAgentLiveProviderRuntime; }
class LiveMediaSessionRuntime;
class MediaSessionIssuanceService;
class MediaSessionRepository;

class LiveMediaSessionController
{
public:
    LiveMediaSessionController(
        MediaSessionRepository& mediaSessionRepository,
        MediaSessionIssuanceService& mediaSessionIssuanceService,
        vdrsuite::agent::BackendAgentLiveProviderRuntime& providerRuntime,
        std::string workspaceRoot);
    ~LiveMediaSessionController();

    ApiResponse handleRequest(
        const std::string& body,
        const std::string& actorId) const;

    std::size_t reapInactiveSessions(int idleTimeoutSeconds) const;

private:
    ApiResponse createSession(
        const std::string& body,
        const std::string& actorId) const;
    ApiResponse stopSession(
        const std::string& body,
        const std::string& actorId) const;

    MediaSessionRepository& mediaSessionRepository_;
    MediaSessionIssuanceService& mediaSessionIssuanceService_;
    vdrsuite::agent::BackendAgentLiveProviderRuntime& providerRuntime_;
    std::unique_ptr<LiveMediaSessionRuntime> runtime_;
};
