#include "RecordingMediaHttpRuntime.h"

#include "ApiRouter.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentLiveProviderRuntime.h"
#include "Database.h"
#include "ContinueWatchingApiRuntime.h"
#include "DeviceMediaProfileRepository.h"
#include "DeviceMediaProfileQuery.h"
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
#include "MediaSessionWorkspace.h"
#include "LocalVdrRecordingSourceResolver.h"
#include "RecordingTrickplayDeviceRequest.h"
#include "RecordingTrickplayPreviewPlan.h"
#include "RecordingTrickplayPreviewRenderer.h"
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
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr const char* MediaSessionWorkspaceRoot =
    "/var/cache/vdr-suite/media-sessions";
constexpr int MediaAccessIdleTimeoutSeconds = 300;
// Fail fast: one thumbnail request may resolve a source, stage a workspace
// and render a frame at a time. Prevent index scans and 1024 symlink trees
// from piling up even before the renderer's own worker gate.
std::mutex trickplayRequestMutex;
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
    DeviceMediaProfileRepository deviceMediaProfiles(database);
    if (!deviceMediaProfiles.ensureSchema()) {
        std::cerr << "Device Media verified-path schema unavailable" << std::endl;
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
            const std::string capabilityCollection = collection + "/capabilities";
            if (body.empty() && target.rfind(capabilityCollection,0) == 0 &&
                (target.size() == capabilityCollection.size() ||
                 target[capabilityCollection.size()] == '?')) {
                if (!verifiedDevice.device)
                    return error(403, "device_identity_required");
                const auto key = parseDeviceMediaProfileQuery(
                    target, capabilityCollection, verifiedDevice.device->deviceId);
                if (!key) return error(400, "invalid_device_media_profile_query");
                const auto stored = deviceMediaProfiles.findVerified(*key);
                if (!stored) return error(503, "device_media_profile_unavailable");
                ApiResponse status;
                status.statusCode = 200;
                status.contentType = "application/json";
                status.headers["Cache-Control"] = "no-store";
                status.headers["X-Content-Type-Options"] = "nosniff";
                status.body = "{\"version\":1,\"profileState\":\"" +
                    std::string(stored->verifiedPaths.empty() ? "unverified" : "verified") +
                    "\",\"verifiedPathCount\":" +
                    std::to_string(stored->verifiedPaths.size()) + "}";
                return status;
            }
            // P1: extend the EXISTING Device playback owner. Every read and
            // mutation resolves the current public rec_ identity through the
            // same admission/grant/backend gates as MediaSession CREATE.
            // Without an explicit Human<->Device binding, the Service actor
            // receives only its OWN progress, never a guessed Human account.
            const std::string progressPath = collection + "/progress";
            if (target == progressPath ||
                target.rfind(progressPath + "?", 0) == 0) {
                std::string backendId;
                std::string publicRecordingId;
                PublicRecordingProgressRequest progress;
                if (body.empty()) {
                    const std::size_t question = target.find('?');
                    if (question == std::string::npos)
                        return error(400, "recording_progress_query_required");
                    const std::string query = target.substr(question + 1U);
                    unsigned count = 0U;
                    for (std::size_t start = 0; start < query.size();) {
                        const auto end = query.find('&', start);
                        const auto pair = query.substr(start,
                            end == std::string::npos ? std::string::npos : end - start);
                        const auto equals = pair.find('=');
                        if (equals == std::string::npos || ++count > 2U)
                            return error(400, "recording_progress_query_invalid");
                        const std::string key = pair.substr(0, equals);
                        const std::string value = pair.substr(equals + 1U);
                        if (key == "backendId" && backendId.empty())
                            backendId = value;
                        else if (key == "recordingId" && publicRecordingId.empty())
                            publicRecordingId = value;
                        else return error(400, "recording_progress_query_invalid");
                        if (end == std::string::npos) break;
                        start = end + 1U;
                    }
                    if (count != 2U || backendId.empty() || publicRecordingId.empty())
                        return error(400, "recording_progress_query_invalid");
                } else {
                    if (target != progressPath)
                        return error(400, "recording_progress_query_invalid");
                    progress = ContinueWatchingApiRuntime::parsePublicProgressRequest(body);
                    if (!progress.valid)
                        return error(400, "recording_progress_payload_invalid");
                    backendId = progress.backendId;
                    publicRecordingId = progress.recordingId;
                }
                const auto allowed = admission.admit(
                    verifiedDevice, backendId, publicRecordingId);
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
                const std::string actorId = verifiedDevice.actor.actorId;
                return body.empty()
                    ? ContinueWatchingApiRuntime::instance().publicProgressGet(
                        actorId, allowed.recording, publicRecordingId)
                    : ContinueWatchingApiRuntime::instance().publicProgressUpdate(
                        actorId, allowed.recording, publicRecordingId, progress);
            }
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

                std::optional<DeviceMediaCapabilityProfile> storedProfile;
                DeviceMediaProfileKey authenticatedPlayer;
                if (request.deviceMediaProfileRequested) {
                    // Device identity is NEVER parsed from request JSON.
                    if (!verifiedDevice.device ||
                        verifiedDevice.device->deviceId.empty())
                        return error(403, "device_identity_required");
                    authenticatedPlayer.deviceId = verifiedDevice.device->deviceId;
                    authenticatedPlayer.appVersion = request.mediaAppVersion;
                    authenticatedPlayer.firmwareVersion = request.mediaFirmwareVersion;
                    authenticatedPlayer.playerVersion = request.mediaPlayerVersion;
                    authenticatedPlayer.engine =
                        request.mediaPlayerEngine == "mse"
                        ? MediaPlaybackEngine::MediaSourceExtensions
                        : request.mediaPlayerEngine == "external"
                            ? MediaPlaybackEngine::External
                            : MediaPlaybackEngine::Native;
                    storedProfile = deviceMediaProfiles.findVerified(authenticatedPlayer);
                    if (!storedProfile || storedProfile->verifiedPaths.empty())
                        return error(409, "device_media_profile_unverified");
                }

                std::string sessionId;
                ApiResponse created = recordingMediaSessionController.createDeviceSession(
                    body, verifiedDevice.actor.actorId, allowed.recording,
                    request.recordingId, "/vdr-suite", "", sessionId,
                    storedProfile ? &*storedProfile : nullptr,
                    request.deviceMediaProfileRequested ? &authenticatedPlayer : nullptr);
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
            // Separate, read-only, per-Session Device Trickplay resource.
            // SecurityHttpGate requires a live Device proof for this whole
            // control namespace; describeOwned RECHECKS persistence-backed
            // identity, credential, exact Session owner and current play
            // grant before any native recording or filesystem is touched.
            if (target.rfind(prefix, 0) == 0 &&
                target.find("/trickplay", prefix.size()) !=
                    std::string::npos) {
                const auto request =
                    RecordingTrickplayDeviceRequestParser().parse(target);
                if (!body.empty() || !request.valid)
                    return error(400, request.reasonCode.empty()
                        ? "trickplay_request_invalid" : request.reasonCode);
                const auto owned =
                    deviceSessions.describeOwned(request.sessionId, verifiedDevice);
                if (!owned) return error(404, "media_session_not_found");
                std::unique_lock<std::mutex> oneRequest(
                    trickplayRequestMutex, std::try_to_lock);
                if (!oneRequest.owns_lock())
                    return error(503, "trickplay_worker_busy");

                // Session resourceId is server-issued (native); no arbitrary
                // filesystem path or public recording ID is accepted here.
                VdrRecording recording;
                if (!recordingQueryService.findRecordingById(
                        owned->backendId, owned->resourceId, recording) ||
                    recording.id != owned->resourceId ||
                    (recording.backendId != owned->backendId &&
                     !(recording.backendId.empty() &&
                       owned->backendId == "default")))
                    return error(404, "recording_not_found");

                LocalVdrRecordingSourceResolver resolver(
                    [recording](const std::string& backendId) {
                        if (recording.backendId != backendId &&
                            !(recording.backendId.empty() &&
                              backendId == "default"))
                            return std::vector<VdrRecording>{};
                        return std::vector<VdrRecording>{recording};
                    });
                const auto source = resolver.resolve(
                    owned->backendId, recording.id);
                if (!source.resolved || source.source.growing)
                    return error(409, "trickplay_source_not_ready");

                // Read the real completed VDR index, NEVER EPG durations,
                // client claims or the rolling HLS playlist clock.
                const int duration =
                    vdrsuite::recording::durationSecondsFromIndex(recording);
                const auto durations =
                    vdrsuite::recording::segmentDurationsSecondsFromIndex(
                        recording, source.source.segmentPaths);
                const auto plan = RecordingTrickplayPreviewPlanner().build(
                    request.positionSeconds, duration,
                    source.source.segmentPaths.size(), durations);
                if (!plan.valid) return error(409, plan.reasonCode);

                // One ephemeral workspace per exact owned Session+bucket.
                // Colliding duplicate requests fail closed; the destructor
                // removes only workspace files it created. Never reuse an
                // HLS worker's live workspace or reveal the host paths.
                MediaSessionWorkspace workspace(MediaSessionWorkspaceRoot);
                const auto prepared = workspace.prepare(
                    "trickplay-" + request.sessionId + "-" +
                        std::to_string(plan.sampleSeconds),
                    source.source.segmentPaths, durations);
                if (!prepared.ready)
                    return error(503, "trickplay_workspace_unavailable");

                const auto frame =
                    RecordingTrickplayPreviewRenderer().render(plan, workspace);
                if (!frame.ready)
                    return error(503, frame.reasonCode.empty()
                        ? "trickplay_frame_not_ready" : frame.reasonCode);

                // Recheck live revocation/session stop after FFmpeg: a device
                // losing access during the bounded render gets NO media bytes.
                if (!deviceSessions.describeOwned(
                        request.sessionId, verifiedDevice))
                    return error(403, "device_media_permission_denied");
                ApiResponse image;
                image.statusCode = 200;
                image.contentType = "image/jpeg";
                image.headers["Cache-Control"] = "no-store";
                image.headers["X-Content-Type-Options"] = "nosniff";
                image.headers["X-Trickplay-Sample-Seconds"] =
                    std::to_string(frame.sampleSeconds);
                image.body = frame.jpeg;
                return image;
            }
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

                // The real Recording duration is frames(index) / fps(info).
                // Never infer it from the rolling HLS playlist or EPG length.
                int durationSeconds = 0;
                VdrRecording indexedRecording;
                if (recordingQueryService.findRecordingById(
                        owned->backendId,
                        owned->resourceId,
                        indexedRecording)) {
                    durationSeconds =
                        vdrsuite::recording::durationSecondsFromIndex(
                            indexedRecording);
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
                    "\",\"playback\":{\"durationSeconds\":" +
                    (durationSeconds > 0
                        ? std::to_string(durationSeconds)
                        : "null") +
                    ",\"resume\":{\"preparing\":" +
                    (preparing ? "true" : "false") +
                    "}}}}";
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

    // Live playback uses a separate Public-v1 Device-only control plane.
    // SecurityHttpGate already verified media.live.play for this backend.
    // The registry rechecks revocation for each media gateway GET.
    apiRouter.setDeviceLivePlaybackHandler(
        [&](const std::string& target, const std::string& body,
            const RequestSecurityContext& verifiedDevice) {
            auto fail = [](int status, const std::string& code) {
                ApiResponse response;
                response.statusCode = status;
                response.contentType = "application/json";
                response.headers["Cache-Control"] = "no-store";
                response.headers["X-Content-Type-Options"] = "nosniff";
                response.body = "{\"error\":{\"code\":\"" + code + "\"}}";
                return response;
            };
            const std::string collection = "/api/v1/live-playback-sessions";
            const std::string prefix = collection + "/";
            auto safeSessionId = [](const std::string& id) {
                return id.size() == 35U && id.rfind("ms_", 0U) == 0U &&
                    std::all_of(id.begin() + 3, id.end(),
                        [](unsigned char ch) {
                            return (ch >= '0' && ch <= '9') ||
                                (ch >= 'a' && ch <= 'f');
                        });
            };
            if (target == collection) {
                if (body.size() > 4096U)
                    return fail(400, "device_live_request_too_large");
                const auto request = LiveMediaSessionRequestParser().parse(body);
                if (!request.valid) return fail(400, "invalid_device_live_request");
                if (!request.replacesSessionId.empty())
                    return fail(400, "device_live_replacement_not_supported");
                std::string sessionId;
                ApiResponse created = liveMediaSessionController.createDeviceSession(
                    body, verifiedDevice.actor.actorId, sessionId);
                if (created.statusCode != 201) return created;
                if (!safeSessionId(sessionId) ||
                    !deviceSessions.addLive(sessionId, request.backendId,
                        verifiedDevice)) {
                    if (safeSessionId(sessionId)) {
                        const std::string stop =
                            "{\"operation\":\"stop\",\"resourceKind\":\"live-channel\","
                            "\"backendId\":\"" + request.backendId +
                            "\",\"sessionId\":\"" + sessionId + "\"}";
                        liveMediaSessionController.handleRequest(
                            stop, verifiedDevice.actor.actorId);
                    }
                    return fail(503, "device_live_owner_unavailable");
                }
                return MediaPlaybackContractResponse::augment(
                    std::move(created), true);
            }
            if (target.rfind(prefix, 0) != 0)
                return fail(404, "device_live_route_not_found");
            const std::string suffix = "/stop";
            if (target.size() > prefix.size() + suffix.size() &&
                target.compare(target.size() - suffix.size(),
                               suffix.size(), suffix) == 0) {
                const std::string id = target.substr(
                    prefix.size(),
                    target.size() - prefix.size() - suffix.size());
                if (!safeSessionId(id)) return fail(404, "media_session_not_found");
                const auto stop = RecordingMediaSessionRequestParser().parseStop(body);
                if (!stop.valid || stop.sessionId != id ||
                    !LiveMediaSessionRequestParser::requestsLiveChannel(body))
                    return fail(400, "invalid_device_live_stop");
                const auto owner = deviceSessions.describeOwnedLive(id, verifiedDevice);
                if (!owner || owner->backendId != stop.backendId)
                    return fail(403, "device_live_session_not_owned");
                ApiResponse ended = liveMediaSessionController.handleRequest(
                    body, verifiedDevice.actor.actorId);
                if (ended.statusCode == 200) {
                    deviceSessions.erase(id);
                    ended.headers.erase("Set-Cookie");
                    ended.headers["Set-Cookie"] =
                        MediaAccessCredentialHttp::expiredPublicV1SessionCookie(
                            id, "/vdr-suite");
                }
                return ended;
            }
            const std::string id = target.substr(prefix.size());
            if (!body.empty() || !safeSessionId(id))
                return fail(404, "device_live_route_not_found");
            const auto owner = deviceSessions.describeOwnedLive(id, verifiedDevice);
            if (!owner) return fail(404, "media_session_not_found");
            if (owner->presentationProfileId != "live-progressive-fmp4")
                return fail(503, "device_live_profile_unavailable");
            ApiResponse response;
            response.statusCode = 200;
            response.contentType = "application/json";
            response.headers["Cache-Control"] = "no-store";
            response.body =
                "{\"mediaSession\":{\"id\":\"" + id +
                "\",\"backendId\":\"" + owner->backendId +
                "\",\"channelId\":\"" + owner->resourceId +
                "\",\"state\":\"ready\","
                "\"presentationProfileId\":\"live-progressive-fmp4\"}}";
            return response;
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
    apiRouter.setDeviceLivePlaybackHandler({});
    MediaTranscodeSettingsApiRuntime::instance().reset();
    return result;
}
