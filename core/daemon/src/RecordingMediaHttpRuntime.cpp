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
#include "MediaTranscodeSettingsApiRuntime.h"
#include "RecordingDirectSourceRegistry.h"
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
    if (!MediaTranscodeSettingsApiRuntime::instance().configure(database)) {
        std::cerr << "failed to initialize media transcode settings runtime" << std::endl;
        return 1;
    }

    MediaRouteLeaseRepository mediaRouteLeaseRepository(database);
    MediaSessionIssuanceService mediaSessionIssuanceService(mediaSessionRepository);
    RecordingDirectSourceRegistry recordingDirectSourceRegistry;
    RecordingMediaSessionController recordingMediaSessionController(
        recordingQueryService,
        mediaSessionRepository,
        mediaSessionIssuanceService,
        recordingDirectSourceRegistry,
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
                // A direct live response is one long authenticated GET. There
                // is no HLS polling to refresh last_seen_at, so liveness is
                // fenced by worker/provider/grant expiry rather than an idle
                // request timeout. Browser disconnect makes the FIFO writer
                // fail and the worker reaper closes the native receiver.
                liveMediaSessionController.reapInactiveSessions(0);
                nextMediaSessionReap = now + MediaSessionReapInterval;
            }
            if (onTick) onTick();
        };

    httpListener.reset();
    httpServer = std::make_unique<MediaGatewayHttpServer>(
        std::move(httpServer),
        mediaAccessGrantAuthenticator,
        mediaRouteLeaseRepository,
        mediaHlsArtifactReader,
        MediaSessionWorkspaceRoot,
        &recordingDirectSourceRegistry);
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
    std::cout << "Media transcode settings runtime initialized" << std::endl;
    std::cout << "vdr-suite-daemon runtime running" << std::endl;
    std::cout << "vdr-suite-daemon serving HTTP on "
              << listenHost << ":" << listenPort << std::endl;

    const int result = httpListener->runUntilStopped();

    httpListener.reset();
    httpServer.reset();
    apiRouter.setRecordingMediaSessionHandler({});
    MediaTranscodeSettingsApiRuntime::instance().reset();
    return result;
}
