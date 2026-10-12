#include "RecordingMediaHttpRuntime.h"

#include "ApiRouter.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentLiveProviderRuntime.h"
#include "Database.h"
#include "DeviceRecordingResumeProjection.h"
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
#include "VdrRecordingDuration.h"

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
            // Device-only track inventory of the currently owned Session.
            // The existing Browser trackStatus computes the source inventory,
            // but its response also contains a native Recording ID: NEVER
            // forward that full response to the paired device.
            const std::string tracksSuffix = "/tracks";
            if (body.empty() &&
                target.rfind(prefix, 0) == 0 &&
                target.size() == prefix.size() + 35U + tracksSuffix.size() &&
                target.compare(prefix.size(), 3U, "ms_") == 0 &&
                target.compare(target.size() - tracksSuffix.size(),
                               tracksSuffix.size(), tracksSuffix) == 0 &&
                std::all_of(target.begin() + prefix.size() + 3U,
                            target.begin() + prefix.size() + 35U,
                            [](unsigned char ch) {
                                return (ch >= '0' && ch <= '9') ||
                                       (ch >= 'a' && ch <= 'f');
                            })) {
                const std::string id = target.substr(prefix.size(), 35U);
                const auto owned = deviceSessions.describeOwned(id, verifiedDevice);
                if (!owned) return error(404, "media_session_not_found");
                const std::string statusRequest =
                    "{\"operation\":\"track-status\",\"backendId\":\"" +
                    owned->backendId + "\",\"sessionId\":\"" + id + "\"}";
                const ApiResponse internal =
                    recordingMediaSessionController.handleRequest(
                        statusRequest, verifiedDevice.actor.actorId);
                if (internal.statusCode != 200)
                    return error(409, "recording_track_status_unavailable");
                const std::string marker = "\"tracks\":";
                const auto at = internal.body.find(marker);
                // Only extract the canonical RecordingMediaTrackContract JSON.
                // It is the last field of the trusted internal mediaSession
                // envelope, which closes with exactly two braces.
                if (at == std::string::npos ||
                    internal.body.size() < at + marker.size() + 4U ||
                    internal.body.size() > 65536U ||
                    internal.body.compare(internal.body.size() - 2U, 2U, "}}") != 0 ||
                    internal.body[at + marker.size()] != '{')
                    return error(503, "recording_track_projection_invalid");
                const std::string tracks = internal.body.substr(
                    at + marker.size(),
                    internal.body.size() - 2U - (at + marker.size()));
                if (tracks.size() < 2U || tracks.front() != '{' ||
                    tracks.back() != '}')
                    return error(503, "recording_track_projection_invalid");
                ApiResponse projected;
                projected.statusCode = 200;
                projected.contentType = "application/json";
                projected.headers["Cache-Control"] = "no-store";
                projected.headers["X-Content-Type-Options"] = "nosniff";
                projected.body =
                    "{\"mediaSession\":{\"id\":\"" + id +
                    "\",\"state\":\"ready\",\"backendId\":\"" +
                    owned->backendId + "\",\"tracks\":" + tracks + "}}";
                return projected;
            }
            // Versioned Device-only GET /{ms-id}. Never return the native
            // Recording ID, worker location, provider URL or media secret.
            if (body.empty() &&
                target.rfind(prefix, 0) == 0 &&
                target.size() == prefix.size() + 35U &&
                target.compare(prefix.size(), 3U, "ms_") == 0 &&
                std::all_of(
                    target.begin() + prefix.size() + 3U,
                    target.end(), [](unsigned char ch) {
                        return (ch >= '0' && ch <= '9') ||
                            (ch >= 'a' && ch <= 'f');
                    }))
            {
                const std::string id = target.substr(prefix.size());
                const auto owned = deviceSessions.describeOwned(
                    id, verifiedDevice);
                if (!owned) return error(404, "media_session_not_found");
                // Both values originate from internally validated issuance.
                // Do not mirror arbitrary client input in this read model.
                const auto profile = owned->presentationProfileId;
                if (profile != "hls-ts" && profile != "hls-fmp4" &&
                    profile != "progressive-direct" &&
                    profile != "progressive-fmp4")
                    return error(503, "media_session_profile_unavailable");
                // Reuse the established first-party index completion path:
                // it updates the Recording cache when VDR's generator finishes.
                // The internal playback-status response contains a native
                // Recording ID; NEVER forward it to a Device client.
                const std::string internalRead =
                    "{\"operation\":\"playback-status\",\"backendId\":\"" +
                    owned->backendId + "\",\"sessionId\":\"" + id + "\"}";
                const ApiResponse indexStatus =
                    recordingMediaSessionController.handleRequest(
                        internalRead, verifiedDevice.actor.actorId);
                const bool preparing =
                    indexStatus.statusCode == 200 &&
                    indexStatus.body.find(
                        "\"resume\":{\"supported\":false,\"preparing\":true") !=
                        std::string::npos;

                // Match Web HLS fallback capability gating. The actual index
                // segment timeline, not merely duration, must be complete.
                // Run the same read-only native resolver as Session CREATE;
                // never send its paths, IDs or fingerprint to the Device.
                DeviceRecordingResumeProjection resume;
                VdrRecording indexedRecording;
                if (recordingQueryService.findRecordingById(
                        owned->backendId,
                        owned->resourceId,
                        indexedRecording)) {
                    const LocalVdrRecordingSourceResolver resolver(
                        [indexedRecording](const std::string& backendId) {
                            if (indexedRecording.backendId != backendId &&
                                !(indexedRecording.backendId.empty() &&
                                  backendId == "default"))
                                return std::vector<VdrRecording>{};
                            return std::vector<VdrRecording>{indexedRecording};
                        });
                    const auto source = resolver.resolve(
                        owned->backendId, owned->resourceId);
                    resume = projectDeviceRecordingResume(
                        indexedRecording, source, profile, preparing);
                }

                ApiResponse response;
                response.statusCode = 200;
                response.contentType = "application/json";
                response.headers["Cache-Control"] = "no-store";
                response.headers["X-Content-Type-Options"] = "nosniff";
                response.body =
                    "{\"mediaSession\":{\"id\":\"" + id +
                    "\",\"state\":\"ready\",\"backendId\":\"" +
                    owned->backendId +
                    "\",\"presentationProfileId\":\"" +
                    profile +
                    "\",\"playback\":" +
                    deviceRecordingResumePlaybackJson(resume) + "}}";
                return response;
            }
            // Device-owned WebVTT selection: reuse the existing subtitle
            // converter, not the Browser /api/media/sessions endpoint.
            // SecurityHttpGate requires Device proof and play grant first.
            const std::string subtitleSuffix = "/subtitles";
            if (target.rfind(prefix, 0) == 0 &&
                target.size() == prefix.size() + 35U + subtitleSuffix.size() &&
                target.compare(target.size() - subtitleSuffix.size(),
                               subtitleSuffix.size(), subtitleSuffix) == 0 &&
                target.compare(prefix.size(), 3U, "ms_") == 0 &&
                std::all_of(target.begin() + prefix.size() + 3U,
                            target.begin() + prefix.size() + 35U,
                            [](unsigned char ch) {
                                return (ch >= '0' && ch <= '9') ||
                                       (ch >= 'a' && ch <= 'f');
                            })) {
                const std::string id = target.substr(prefix.size(), 35U);
                const auto selection =
                    RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(body);
                if (!selection.valid || selection.sessionId != id)
                    return error(400, "invalid_device_subtitle_selection");
                if (!deviceSessions.owned(id, selection.backendId, verifiedDevice))
                    return error(403, "device_media_session_not_owned");
                const std::string safeRequest =
                    "{\"operation\":\"select-subtitle-track\",\"backendId\":\"" +
                    selection.backendId + "\",\"sessionId\":\"" + id +
                    "\",\"subtitleTrackId\":\"" + selection.subtitleTrackId +
                    "\",\"streamBasePositionSeconds\":" +
                    std::to_string(selection.streamBasePositionSeconds) + "}";
                ApiResponse result = recordingMediaSessionController.handleRequest(
                    safeRequest, verifiedDevice.actor.actorId);
                result.headers.erase("Set-Cookie");
                result.headers["Cache-Control"] = "no-store";
                result.headers["X-Content-Type-Options"] = "nosniff";
                // Restrict the Device delivery only; do not modify the
                // established Browser subtitle controller/converter.
                if (result.statusCode == 200 &&
                    (result.body.size() > 1024U * 1024U ||
                     (selection.subtitleTrackId != "off" &&
                      (result.contentType.rfind("text/vtt", 0) != 0 ||
                       result.body.rfind("WEBVTT", 0) != 0)) ||
                     (selection.subtitleTrackId == "off" &&
                      result.contentType != "application/json")))
                    return error(503, "device_subtitle_delivery_invalid");
                return result;
            }
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
