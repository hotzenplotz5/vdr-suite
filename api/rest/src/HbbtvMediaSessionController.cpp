#include "HbbtvMediaSessionController.h"

#include "HbbtvMediaSessionRuntime.h"
#include "MediaAccessCredentialHttp.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"

#include <string>
#include <utility>

namespace
{
constexpr int MediaSessionLifetimeSeconds = 3600;
constexpr const char* PendingProfileId =
    "hbbtv-media-negotiating";
constexpr const char* ProviderId =
    "vdr-plugin-web-hbbtv";

std::string jsonEscape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char character : value)
    {
        if (character == '"')
            result += "\\\"";
        else if (character == '\\')
            result += "\\\\";
        else if (character >= 0x20U)
            result.push_back(
                static_cast<char>(character));
    }
    return result;
}

ApiResponse jsonError(
    int statusCode,
    const std::string& code)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType =
        "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] =
        "nosniff";
    response.body =
        "{\"error\":{\"code\":\"" +
        jsonEscape(code) + "\"}}";
    return response;
}

ClientMediaCapabilities browserCapabilities()
{
    ClientMediaCapabilities client;
    client.protocols = {
        MediaDeliveryProtocol::Progressive};
    client.containers = {MediaContainer::Fmp4};
    client.videoCodecs = {MediaCodec::H264};
    client.audioCodecs = {MediaCodec::Aac};
    client.supportsByteRanges = false;
    client.maxVideoWidth = 1920;
    client.maxVideoHeight = 1080;
    client.maxAudioChannels = 2;
    return client;
}

ApiResponse stopped(
    const std::string& hbbtvSessionId,
    const std::string& mediaSessionId)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType =
        "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] =
        "nosniff";

    if (!mediaSessionId.empty())
    {
        const std::string cookie =
            MediaAccessCredentialHttp::
                expiredSessionCookie(
                    mediaSessionId);
        if (!cookie.empty())
            response.headers["Set-Cookie"] =
                cookie;
    }

    response.body =
        "{\"mediaSession\":{"
        "\"id\":\"" +
        jsonEscape(mediaSessionId) + "\","
        "\"resourceKind\":\"hbbtv-media\","
        "\"state\":\"ended\","
        "\"hbbtvSessionId\":\"" +
        jsonEscape(hbbtvSessionId) +
        "\"}}";
    return response;
}
}

HbbtvMediaSessionController::
HbbtvMediaSessionController(
    MediaSessionRepository& mediaSessionRepository,
    MediaSessionIssuanceService& mediaSessionIssuanceService,
    std::string workspaceRoot)
    : mediaSessionRepository_(mediaSessionRepository),
      mediaSessionIssuanceService_(
          mediaSessionIssuanceService),
      runtime_(
          std::make_unique<HbbtvMediaSessionRuntime>(
              mediaSessionRepository,
              std::move(workspaceRoot)))
{
}

HbbtvMediaSessionController::
~HbbtvMediaSessionController() = default;

std::size_t
HbbtvMediaSessionController::reapInactiveSessions(
    int idleTimeoutSeconds) const
{
    return runtime_->reapInactive(
        idleTimeoutSeconds);
}

ApiResponse
HbbtvMediaSessionController::handleMutation(
    const HbbtvMediaSessionMutationRequest& request) const
{
    if (request.actorId.empty() ||
        request.backendId.empty() ||
        request.sessionId.empty())
    {
        return jsonError(
            403,
            "hbbtv_media_request_context_invalid");
    }

    if (request.operation ==
        HbbtvMediaSessionMutationOperation::Start)
    {
        return start(request);
    }
    return stop(
        request,
        "hbbtv_media_client_stop");
}

ApiResponse HbbtvMediaSessionController::start(
    const HbbtvMediaSessionMutationRequest& request) const
{
    if (request.mediaRevision == 0 ||
        !request.media.available ||
        request.media.mediaRevision !=
            request.mediaRevision ||
        request.media.consumerConnected ||
        (request.media.state !=
             HbbtvMediaSourceState::Streaming &&
         request.media.state !=
             HbbtvMediaSourceState::Paused))
    {
        return jsonError(
            409,
            "hbbtv_media_source_not_startable");
    }

    const auto active =
        runtime_->activeForApplicationSession(
            request.sessionId);
    if (active.has_value())
    {
        if (active->mediaRevision ==
            request.mediaRevision)
        {
            return jsonError(
                409,
                "hbbtv_media_revision_already_owned");
        }
        if (!runtime_->stop(
                active->mediaSessionId,
                "hbbtv_media_revision_replaced"))
        {
            return jsonError(
                503,
                "hbbtv_media_previous_session_stop_failed");
        }
    }

    MediaSessionIssuanceRequest issuanceRequest;
    issuanceRequest.actorId = request.actorId;
    issuanceRequest.backendId = request.backendId;
    issuanceRequest.resourceKind = "hbbtv-media";
    issuanceRequest.resourceId =
        request.sessionId + ":" +
        std::to_string(request.mediaRevision);
    issuanceRequest.presentationProfileId =
        PendingProfileId;
    issuanceRequest.providerId = ProviderId;
    issuanceRequest.lifetimeSeconds =
        MediaSessionLifetimeSeconds;

    auto issuance =
        mediaSessionIssuanceService_.issue(
            issuanceRequest);
    if (!issuance.issued)
    {
        return jsonError(
            503,
            issuance.reasonCode.empty()
                ? "hbbtv_media_session_issue_failed"
                : issuance.reasonCode);
    }

    const std::string cookie =
        MediaAccessCredentialHttp::sessionCookie(
            issuance.session.sessionId,
            issuance.session.accessCredential,
            MediaSessionLifetimeSeconds);
    if (cookie.empty())
    {
        mediaSessionRepository_.endBundle(
            issuance.session.sessionId,
            "credential_transport_failed");
        issuance.session.clearSecret();
        return jsonError(
            500,
            "media_access_credential_transport_failed");
    }

    const HbbtvMediaSessionProvisionResult provision =
        runtime_->provisionStream(
            issuance.session.sessionId,
            issuance.session.workspaceId,
            issuance.session.grantId,
            request.sessionId,
            request.backendId,
            request.media,
            browserCapabilities());
    if (!provision.ready)
    {
        issuance.session.clearSecret();
        return jsonError(
            503,
            provision.reasonCode.empty()
                ? "hbbtv_media_stream_provision_failed"
                : provision.reasonCode);
    }

    if (provision.presentation.profileId.empty() ||
        !mediaSessionRepository_.
            updateProvisioningPresentationProfile(
                issuance.session.sessionId,
                provision.presentation.profileId))
    {
        runtime_->stop(
            issuance.session.sessionId,
            "presentation_profile_persistence_failed");
        issuance.session.clearSecret();
        return jsonError(
            503,
            "presentation_profile_persistence_failed");
    }

    if (!mediaSessionRepository_.activateBundle(
            issuance.session.sessionId))
    {
        runtime_->stop(
            issuance.session.sessionId,
            "media_session_activation_failed");
        issuance.session.clearSecret();
        return jsonError(
            503,
            "media_session_activation_failed");
    }

    ApiResponse response;
    response.statusCode = 201;
    response.contentType =
        "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] =
        "nosniff";
    response.headers["Set-Cookie"] = cookie;

    response.body =
        "{\"mediaSession\":{"
        "\"id\":\"" +
        jsonEscape(
            issuance.session.sessionId) + "\","
        "\"resourceKind\":\"hbbtv-media\","
        "\"state\":\"ready\","
        "\"backendId\":\"" +
        jsonEscape(request.backendId) + "\","
        "\"hbbtvSessionId\":\"" +
        jsonEscape(request.sessionId) + "\","
        "\"mediaRevision\":" +
        std::to_string(
            request.mediaRevision) + ","
        "\"presentationProfileId\":\"" +
        jsonEscape(
            provision.presentation.profileId) + "\","
        "\"fullscreen\":" +
        std::string(
            request.media.fullscreen
                ? "true" : "false") + ","
        "\"geometry\":{\"x\":" +
        std::to_string(request.media.x) +
        ",\"y\":" +
        std::to_string(request.media.y) +
        ",\"width\":" +
        std::to_string(request.media.width) +
        ",\"height\":" +
        std::to_string(request.media.height) +
        "},"
        "\"mediaPath\":\"/api/media/sessions/" +
        jsonEscape(
            issuance.session.sessionId) +
        "/live/stream.mp4\","
        "\"expiresAt\":\"" +
        jsonEscape(
            issuance.session.expiresAt) +
        "\"}}";

    issuance.session.clearSecret();
    return response;
}

ApiResponse HbbtvMediaSessionController::stop(
    const HbbtvMediaSessionMutationRequest& request,
    const std::string& reasonCode) const
{
    const auto active =
        runtime_->activeForApplicationSession(
            request.sessionId);
    if (!active.has_value())
        return stopped(
            request.sessionId,
            "");

    if (!runtime_->stop(
            active->mediaSessionId,
            reasonCode))
    {
        return jsonError(
            503,
            "hbbtv_media_session_stop_failed");
    }

    return stopped(
        request.sessionId,
        active->mediaSessionId);
}
