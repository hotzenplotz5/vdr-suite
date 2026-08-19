#include "LiveMediaSessionController.h"

#include "BackendAgentLiveProviderRuntime.h"
#include "LiveMediaSessionRequestParser.h"
#include "LiveMediaSessionRuntime.h"
#include "MediaAccessCredentialHttp.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "RecordingMediaSessionRequestParser.h"

#include <string>
#include <utility>

namespace
{
constexpr int MediaSessionLifetimeSeconds = 21600;
constexpr const char* NativeLiveProviderId = "suitebridge-native-live";
constexpr const char* PendingLiveProfileId = "live-hls-negotiating";

ApiResponse jsonError(int statusCode, const std::string& code)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.body = "{\"error\":{\"code\":\"" + code + "\"}}";
    return response;
}

std::string jsonEscape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char character : value) {
        if (character == '"') result += "\\\"";
        else if (character == '\\') result += "\\\\";
        else if (character >= 0x20U) result.push_back(static_cast<char>(character));
    }
    return result;
}

const char* codecName(MediaCodec codec)
{
    switch (codec) {
    case MediaCodec::H264: return "h264";
    case MediaCodec::H265: return "h265";
    case MediaCodec::Mpeg2Video: return "mpeg2video";
    case MediaCodec::Aac: return "aac";
    case MediaCodec::Ac3: return "ac3";
    case MediaCodec::Eac3: return "eac3";
    case MediaCodec::Dts: return "dts";
    case MediaCodec::MpegAudio: return "mpeg-audio";
    case MediaCodec::None: return "none";
    case MediaCodec::Unknown: return "unknown";
    }
    return "unknown";
}

const char* adaptationName(MediaAdaptationClass value)
{
    switch (value) {
    case MediaAdaptationClass::PassThrough: return "pass-through";
    case MediaAdaptationClass::Remux: return "remux";
    case MediaAdaptationClass::Transcode: return "transcode";
    }
    return "unknown";
}

ApiResponse stoppedResponse(const std::string& sessionId)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    const std::string expiredCookie = MediaAccessCredentialHttp::expiredSessionCookie(sessionId);
    if (!expiredCookie.empty()) response.headers["Set-Cookie"] = expiredCookie;
    response.body = "{\"mediaSession\":{\"id\":\"" + jsonEscape(sessionId) +
        "\",\"resourceKind\":\"live-channel\",\"state\":\"ended\"}}";
    return response;
}
}

LiveMediaSessionController::LiveMediaSessionController(
    MediaSessionRepository& mediaSessionRepository,
    MediaSessionIssuanceService& mediaSessionIssuanceService,
    vdrsuite::agent::BackendAgentLiveProviderRuntime& providerRuntime,
    std::string workspaceRoot)
    : mediaSessionRepository_(mediaSessionRepository),
      mediaSessionIssuanceService_(mediaSessionIssuanceService),
      providerRuntime_(providerRuntime),
      runtime_(std::make_unique<LiveMediaSessionRuntime>(
          mediaSessionRepository, providerRuntime, std::move(workspaceRoot)))
{
}

LiveMediaSessionController::~LiveMediaSessionController() = default;

std::size_t LiveMediaSessionController::reapInactiveSessions(int idleTimeoutSeconds) const
{
    return runtime_->reapInactive(idleTimeoutSeconds);
}

ApiResponse LiveMediaSessionController::handleRequest(
    const std::string& body,
    const std::string& actorId) const
{
    if (actorId.empty()) return jsonError(401, "media_actor_required");
    const auto stopRequest = RecordingMediaSessionRequestParser().parseStop(body);
    if (stopRequest.valid) return stopSession(body, actorId);
    if (stopRequest.reasonCode != "media_session_stop_not_requested")
        return jsonError(400, stopRequest.reasonCode.empty()
            ? "invalid_media_session_operation" : stopRequest.reasonCode);
    return createSession(body, actorId);
}

ApiResponse LiveMediaSessionController::stopSession(
    const std::string& body,
    const std::string& actorId) const
{
    if (!LiveMediaSessionRequestParser::requestsLiveChannel(body))
        return jsonError(400, "invalid_live_resource_kind");
    const auto request = RecordingMediaSessionRequestParser().parseStop(body);
    if (!request.valid) return jsonError(400, request.reasonCode.empty()
        ? "invalid_media_session_stop_request" : request.reasonCode);
    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId ||
        stored->resourceKind != "live-channel")
        return jsonError(404, "media_session_not_found");
    if (stored->actorId != actorId) return jsonError(403, "media_session_not_owned");
    if (stored->state == "ended" || stored->state == "failed")
        return stoppedResponse(request.sessionId);
    if (!runtime_->stop(request.sessionId, "client_closed")) {
        mediaSessionRepository_.endBundle(request.sessionId, "client_stop_runtime_unavailable");
        return jsonError(503, "media_session_stop_failed");
    }
    return stoppedResponse(request.sessionId);
}

ApiResponse LiveMediaSessionController::createSession(
    const std::string& body,
    const std::string& actorId) const
{
    const auto request = LiveMediaSessionRequestParser().parse(body);
    if (!request.valid) return jsonError(400, request.reasonCode.empty()
        ? "invalid_live_media_session_request" : request.reasonCode);

    // Validate B completely through authority/fences before touching A. This
    // does not tune or attach a receiver.
    const auto preparation = providerRuntime_.prepare(request.backendId, request.channelId);
    if (!preparation.valid) return jsonError(503, preparation.reasonCode.empty()
        ? "live_provider_authority_unavailable" : preparation.reasonCode);

    if (!request.replacesSessionId.empty()) {
        const auto replaced = mediaSessionRepository_.findSession(request.replacesSessionId);
        if (!replaced.has_value() || replaced->backendId != request.backendId ||
            replaced->resourceKind != "live-channel")
            return jsonError(404, "replaced_live_session_not_found");
        if (replaced->actorId != actorId)
            return jsonError(403, "replaced_live_session_not_owned");
        if (replaced->state != "ended" && replaced->state != "failed" &&
            !runtime_->stop(request.replacesSessionId, "channel_replaced"))
            return jsonError(503, "replaced_live_session_stop_failed");
    }

    MediaSessionIssuanceRequest issuanceRequest;
    issuanceRequest.actorId = actorId;
    issuanceRequest.backendId = request.backendId;
    issuanceRequest.resourceKind = "live-channel";
    issuanceRequest.resourceId = request.channelId;
    issuanceRequest.presentationProfileId = PendingLiveProfileId;
    issuanceRequest.providerId = NativeLiveProviderId;
    issuanceRequest.lifetimeSeconds = MediaSessionLifetimeSeconds;
    auto issuance = mediaSessionIssuanceService_.issue(issuanceRequest);
    if (!issuance.issued) return jsonError(503, issuance.reasonCode.empty()
        ? "media_session_issue_failed" : issuance.reasonCode);

    const std::string cookie = MediaAccessCredentialHttp::sessionCookie(
        issuance.session.sessionId,
        issuance.session.accessCredential,
        MediaSessionLifetimeSeconds);
    if (cookie.empty()) {
        mediaSessionRepository_.endBundle(
            issuance.session.sessionId, "credential_transport_failed");
        issuance.session.clearSecret();
        return jsonError(500, "media_access_credential_transport_failed");
    }

    const auto provision = runtime_->provisionHls(
        issuance.session.sessionId,
        issuance.session.workspaceId,
        issuance.session.leaseId,
        issuance.session.grantId,
        preparation,
        request.capabilities);
    if (!provision.ready) {
        issuance.session.clearSecret();
        return jsonError(503, provision.reasonCode.empty()
            ? "live_media_hls_provision_failed" : provision.reasonCode);
    }
    if (provision.presentation.profileId.empty() ||
        !mediaSessionRepository_.updateProvisioningPresentationProfile(
            issuance.session.sessionId, provision.presentation.profileId)) {
        runtime_->stop(issuance.session.sessionId, "presentation_profile_persistence_failed");
        issuance.session.clearSecret();
        return jsonError(503, "presentation_profile_persistence_failed");
    }

    const MediaVideoStreamDescriptor* video = provision.source.videoStreams.empty()
        ? nullptr : &provision.source.videoStreams.front();
    const MediaAudioStreamDescriptor* audio = provision.source.audioStreams.empty()
        ? nullptr : &provision.source.audioStreams.front();

    ApiResponse response;
    response.statusCode = 201;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["Set-Cookie"] = cookie;
    response.body =
        "{\"mediaSession\":{"
        "\"id\":\"" + jsonEscape(issuance.session.sessionId) + "\"," +
        "\"resourceKind\":\"live-channel\"," +
        "\"state\":\"ready\"," +
        "\"backendId\":\"" + jsonEscape(request.backendId) + "\"," +
        "\"channelId\":\"" + jsonEscape(request.channelId) + "\"," +
        "\"presentationProfileId\":\"" + jsonEscape(provision.presentation.profileId) + "\"," +
        "\"adaptation\":\"" + adaptationName(provision.presentation.adaptationClass) + "\"," +
        "\"videoCodec\":\"" + (video == nullptr ? std::string("none") : codecName(video->codec)) + "\"," +
        "\"audioCodec\":\"" + (audio == nullptr ? std::string("none") : codecName(audio->codec)) + "\"," +
        "\"width\":" + std::to_string(video == nullptr ? 0 : video->width) + "," +
        "\"height\":" + std::to_string(video == nullptr ? 0 : video->height) + "," +
        "\"interlaced\":" + std::string(video != nullptr && video->interlaced ? "true" : "false") + "," +
        "\"mediaPath\":\"/api/media/sessions/" +
            jsonEscape(issuance.session.sessionId) + "/hls/master.m3u8\"," +
        "\"expiresAt\":\"" + jsonEscape(issuance.session.expiresAt) + "\"}}";
    issuance.session.clearSecret();
    return response;
}
