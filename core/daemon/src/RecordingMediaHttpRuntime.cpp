#include "RecordingMediaHttpRuntime.h"

#include "ApiRouter.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentLiveProviderRuntime.h"
#include "Database.h"
#include "IHttpServer.h"
#include "LiveMediaSessionController.h"
#include "LiveMediaSessionRequestParser.h"
#include "HbbtvApiRuntime.h"
#include "HbbtvMediaSessionController.h"
#include "MediaAccessGrantAuthenticator.h"
#include "MediaGatewayHttpServer.h"
#include "MediaHlsArtifactReader.h"
#include "MediaPlaybackContractResponse.h"
#include "MediaRouteLeaseRepository.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "PublicDevicePlaybackRegistry.h"
#include "PublicRecordingDevicePlaybackAdmission.h"
#include "PublicRecordingIdentityRepository.h"
#include "PublicRecordingPlaybackTargetResolver.h"
#include "RecordingMediaSessionRequestParser.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"
#include "VdrRecordingCacheRepository.h"
#include "MediaAccessCredentialHttp.h"
#include "MediaTranscodeSettingsApiRuntime.h"
#include "RecordingDirectSourceRegistry.h"
#include "RecordingMediaSessionController.h"
#include "SimpleHttpListener.h"
#include "SuiteBridgeLocalControlTransport.h"
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
    vdrsuite::agent::SuiteBridgeSvdrpTransport suiteBridgeCompatibilityTransport;
    vdrsuite::agent::SuiteBridgeLocalControlTransport suiteBridgeLocalControlTransport;
    vdrsuite::agent::SuiteBridgePrioritizedLiveTransport suiteBridgeLiveTransport(
        suiteBridgeLocalControlTransport,
        suiteBridgeCompatibilityTransport);
    vdrsuite::agent::BackendAgentLiveProviderRuntime liveProviderRuntime(
        agentRepository, commandRepository, suiteBridgeLiveTransport);
    LiveMediaSessionController liveMediaSessionController(
        mediaSessionRepository,
        mediaSessionIssuanceService,
        liveProviderRuntime,
        MediaSessionWorkspaceRoot);
    HbbtvMediaSessionController hbbtvMediaSessionController(
        mediaSessionRepository,
        mediaSessionIssuanceService,
        MediaSessionWorkspaceRoot);
    HbbtvApiRuntime::instance().setMediaSessionHandler(
        [&hbbtvMediaSessionController](
            const HbbtvMediaSessionMutationRequest& request) {
            return hbbtvMediaSessionController.handleMutation(
                request);
        });

    SecurityIdentityRepository securityIdentities(database);
    SecurityPermissionGrantRepository securityGrants(database);
    PublicRecordingIdentityRepository recordingIdentities(database);
    VdrRecordingCacheRepository recordingCache(database);
    if (!securityIdentities.ensureSchema() || !securityGrants.ensureSchema() ||
        !recordingIdentities.ensureSchema() || !recordingCache.ensureSchema()) {
        std::cerr << "Device Playback security persistence unavailable" << std::endl;
        return 1;
    }
    PublicRecordingPlaybackTargetResolver targetResolver(
        recordingIdentities, recordingCache);
    PublicRecordingDevicePlaybackAdmission admission(
        targetResolver, [&recordingCache](const std::string& backend) {
            return recordingCache.statusForBackend(backend).state == "ready";
        });
    PublicDevicePlaybackRegistry deviceSessions(
        mediaSessionRepository, securityIdentities, securityGrants);

    MediaAccessGrantAuthenticator mediaAccessGrantAuthenticator(
        mediaSessionRepository,
        MediaAccessIdleTimeoutSeconds);
    MediaHlsArtifactReader mediaHlsArtifactReader(MediaSessionWorkspaceRoot);

    apiRouter.setRecordingMediaSessionHandler(
        [&recordingMediaSessionController, &liveMediaSessionController](
            const std::string& body,
            const std::string& actorRef)
        {
            const bool liveResource =
                LiveMediaSessionRequestParser::requestsLiveChannel(body);
            ApiResponse response = liveResource
                ? liveMediaSessionController.handleRequest(body, actorRef)
                : recordingMediaSessionController.handleRequest(body, actorRef);
            return MediaPlaybackContractResponse::augment(
                std::move(response),
                liveResource);
        });

    // Separate versioned Device control plane. SecurityHttpGate has already
    // verified the live Device proof and media.recording.play@backend; the
    // domain Admission checks it again before native ID resolution.
    apiRouter.setDeviceRecordingPlaybackHandler(
        [&](const std::string& target, const std::string& body,
            const RequestSecurityContext& verifiedDevice) {
            auto error = [](int code, const std::string& reason) {
                ApiResponse r;
                r.statusCode = code;
                r.headers["Cache-Control"] = "no-store";
                r.body = "{\"error\":{\"code\":\"" + reason + "\"}}";
                return r;
            };
            const std::string collection =
                "/api/v1/recording-playback-sessions";
            if (target == collection) {
                const auto request = RecordingMediaSessionRequestParser().parse(body);
                if (!request.valid) return error(400,
                    request.reasonCode.empty() ? "invalid_playback_request" :
                    request.reasonCode);
                const auto allowed = admission.admit(
                    verifiedDevice, request.backendId, request.recordingId);
                using Status = PublicRecordingDevicePlaybackAdmissionStatus;
                if (allowed.status == Status::unauthenticated)
                    return error(401, "unauthorized");
                if (allowed.status == Status::forbidden)
                    return error(403, "forbidden");
                if (allowed.status == Status::invalidRequest)
                    return error(400, "invalid_recording_id");
                if (allowed.status == Status::notFound)
                    return error(404, "recording_not_found");
                if (allowed.status != Status::ready)
                    return error(503, "recording_backend_unavailable");

                std::string sessionId;
                ApiResponse created = recordingMediaSessionController.createDeviceSession(
                    body, verifiedDevice.actor.actorId, allowed.recording,
                    request.recordingId, "/vdr-suite", "", sessionId);
                if (created.statusCode != 201) return created;
                if (!deviceSessions.add(sessionId, request.backendId, verifiedDevice)) {
                    const std::string stop =
                        "{\"operation\":\"stop\",\"backendId\":\"" +
                        request.backendId + "\",\"sessionId\":\"" +
                        sessionId + "\"}";
                    recordingMediaSessionController.handleRequest(
                        stop, verifiedDevice.actor.actorId);
                    return error(503, "device_playback_owner_unavailable");
                }
                return created;
            }
            const std::string prefix = collection + "/";
            if (target.rfind(prefix, 0) == 0 &&
                target.size() > prefix.size() + 5U &&
                target.compare(target.size() - 5U, 5U, "/stop") == 0) {
                const std::string sessionId =
                    target.substr(prefix.size(), target.size() -
                                  prefix.size() - 5U);
                const auto stop = RecordingMediaSessionRequestParser().parseStop(body);
                if (!stop.valid || stop.sessionId != sessionId)
                    return error(400, "invalid_media_session_stop");
                if (!deviceSessions.owned(
                        sessionId, stop.backendId, verifiedDevice))
                    return error(403, "device_media_session_not_owned");
                ApiResponse ended = recordingMediaSessionController.handleRequest(
                    body, verifiedDevice.actor.actorId);
                deviceSessions.erase(sessionId);
                ended.headers.erase("Set-Cookie");
                const std::string expired =
                    MediaAccessCredentialHttp::expiredPublicV1SessionCookie(
                        sessionId, "");
                if (!expired.empty()) ended.headers["Set-Cookie"] = expired;
                return ended;
            }
            return error(404, "device_playback_route_not_found");
        });

    auto nextMediaSessionReap = std::chrono::steady_clock::now();
    auto mediaRuntimeTick =
        [&recordingMediaSessionController,
         &liveMediaSessionController,
         &hbbtvMediaSessionController,
         &deviceSessions,
         nextMediaSessionReap,
         onTick = std::move(onTick)]() mutable {
            const auto now = std::chrono::steady_clock::now();
            if (now >= nextMediaSessionReap) {
                recordingMediaSessionController.reapInactiveSessions(
                    MediaAccessIdleTimeoutSeconds);
                deviceSessions.pruneTerminal();
                // A direct live response is one long authenticated GET. There
                // is no HLS polling to refresh last_seen_at, so liveness is
                // fenced by worker/provider/grant expiry rather than an idle
                // request timeout. Browser disconnect makes the FIFO writer
                // fail and the worker reaper closes the native receiver.
                liveMediaSessionController.reapInactiveSessions(0);
                hbbtvMediaSessionController.reapInactiveSessions(0);
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
        &recordingDirectSourceRegistry,
        [&deviceSessions](const std::string& sessionId,
            const std::string& actorId,
            const std::string& backendId, bool publicV1Path) {
            return deviceSessions.authorized(
                sessionId, actorId, backendId, publicV1Path);
        });
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
    HbbtvApiRuntime::instance().setMediaSessionHandler({});
    httpServer.reset();
    apiRouter.setRecordingMediaSessionHandler({});
    apiRouter.setDeviceRecordingPlaybackHandler({});
    MediaTranscodeSettingsApiRuntime::instance().reset();
    return result;
}
