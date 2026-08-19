#include "RecordingMediaHttpRuntime.h"

#include "ApiRouter.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentLiveProviderRuntime.h"
#include "Database.h"
#include "IHttpServer.h"
#include "LiveMediaSessionController.h"
#include "LiveMediaSessionRequestParser.h"
#include "MediaAccessGrantAuthenticator.h"
#include "MediaGatewayHttpServer.h"
#include "MediaHlsArtifactReader.h"
#include "MediaRouteLeaseRepository.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "RecordingMediaSessionController.h"
#include "SimpleHttpListener.h"
#include "SuiteBridgeSvdrpTransport.h"
#include "VdrRecordingQueryService.h"

#include <chrono>
#include <iostream>
#include <utility>

namespace
{

constexpr const char* MediaSessionWorkspaceRoot =
    "/var/cache/vdr-suite/media-sessions";
constexpr int MediaAccessIdleTimeoutSeconds = 300;
constexpr auto MediaSessionReapInterval = std::chrono::seconds(5);

}

int runRecordingMediaHttpRuntime(
    Database& database,
    ApiRouter& apiRouter,
    VdrRecordingQueryService& recordingQueryService,
    std::unique_ptr<IHttpServer>& httpServer,
    std::unique_ptr<SimpleHttpListener>& httpListener,
    const std::string& listenHost,
    int listenPort,
    std::function<bool()> shouldStop,
    std::function<void()> onTick)
{
    MediaSessionRepository mediaSessionRepository(database);
    if (!mediaSessionRepository.ensureSchema()) {
        std::cerr << "failed to initialize MediaSession schema" << std::endl;
        return 1;
    }
    if (!mediaSessionRepository.recoverNonTerminalBundles()) {
        std::cerr << "failed to recover MediaSession runtime ownership" << std::endl;
        return 1;
    }

    MediaRouteLeaseRepository mediaRouteLeaseRepository(database);
    MediaSessionIssuanceService mediaSessionIssuanceService(mediaSessionRepository);
    RecordingMediaSessionController recordingMediaSessionController(
        recordingQueryService,
        mediaSessionRepository,
        mediaSessionIssuanceService,
        MediaSessionWorkspaceRoot);

    BackendAgentRepository agentRepository(database);
    BackendAgentCommandRepository commandRepository(database);
    vdrsuite::agent::SuiteBridgeSvdrpTransport suiteBridgeTransport;
    vdrsuite::agent::BackendAgentLiveProviderRuntime liveProviderRuntime(
        agentRepository, commandRepository, suiteBridgeTransport);
    LiveMediaSessionController liveMediaSessionController(
        mediaSessionRepository,
        mediaSessionIssuanceService,
        liveProviderRuntime,
        MediaSessionWorkspaceRoot);

    MediaAccessGrantAuthenticator mediaAccessGrantAuthenticator(
        mediaSessionRepository,
        MediaAccessIdleTimeoutSeconds);
    MediaHlsArtifactReader mediaHlsArtifactReader(MediaSessionWorkspaceRoot);

    apiRouter.setRecordingMediaSessionHandler(
        [&recordingMediaSessionController, &liveMediaSessionController](
            const std::string& body,
            const std::string& actorRef)
        {
            if (LiveMediaSessionRequestParser::requestsLiveChannel(body)) {
                return liveMediaSessionController.handleRequest(body, actorRef);
            }
            return recordingMediaSessionController.handleRequest(body, actorRef);
        });

    auto nextMediaSessionReap = std::chrono::steady_clock::now();
    auto mediaRuntimeTick =
        [&recordingMediaSessionController,
         &liveMediaSessionController,
         nextMediaSessionReap,
         onTick = std::move(onTick)]() mutable {
            const auto now = std::chrono::steady_clock::now();
            if (now >= nextMediaSessionReap) {
                recordingMediaSessionController.reapInactiveSessions(
                    MediaAccessIdleTimeoutSeconds);
                liveMediaSessionController.reapInactiveSessions(
                    MediaAccessIdleTimeoutSeconds);
                nextMediaSessionReap = now + MediaSessionReapInterval;
            }
            if (onTick) onTick();
        };

    httpListener.reset();
    httpServer = std::make_unique<MediaGatewayHttpServer>(
        std::move(httpServer),
        mediaAccessGrantAuthenticator,
        mediaRouteLeaseRepository,
        mediaHlsArtifactReader);
    httpListener = std::make_unique<SimpleHttpListener>(
        listenHost,
        listenPort,
        *httpServer,
        std::move(shouldStop),
        std::move(mediaRuntimeTick));

    std::cout << "MediaSession persistence and restart recovery initialized" << std::endl;
    std::cout << "Media Gateway runtime initialized" << std::endl;
    std::cout << "Recording MediaSession API runtime initialized" << std::endl;
    std::cout << "Live MediaSession API runtime initialized" << std::endl;
    std::cout << "vdr-suite-daemon runtime running" << std::endl;
    std::cout << "vdr-suite-daemon serving HTTP on "
              << listenHost << ":" << listenPort << std::endl;

    const int result = httpListener->runUntilStopped();

    httpListener.reset();
    httpServer.reset();
    apiRouter.setRecordingMediaSessionHandler({});
    return result;
}