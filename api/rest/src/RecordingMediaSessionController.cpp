#include "RecordingMediaSessionController.h"

#include "MediaAccessCredentialHttp.h"
#include "MediaSessionRepository.h"
#include "RecordingDirectSourceRegistry.h"
#include "RecordingMediaSessionRequestParser.h"
#include "RecordingMediaSessionRuntime.h"
#include "VdrRecordingIndexUpdater.h"
#include "VdrRecordingQueryService.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace
{

std::string jsonEscape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char character : value) {
        switch (character) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (character < 0x20) {
                static constexpr char Hex[] = "0123456789abcdef";
                result += "\\u00";
                result.push_back(Hex[(character >> 4) & 0x0f]);
                result.push_back(Hex[character & 0x0f]);
            }
            else {
                result.push_back(static_cast<char>(character));
            }
        }
    }
    return result;
}

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

ApiResponse stoppedResponse(const std::string& sessionId)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    const std::string expiredCookie =
        MediaAccessCredentialHttp::expiredSessionCookie(sessionId);
    if (!expiredCookie.empty()) {
        response.headers["Set-Cookie"] = expiredCookie;
    }
    response.body =
        "{\"mediaSession\":{\"id\":\"" +
        jsonEscape(sessionId) +
        "\",\"state\":\"ended\"}}";
    return response;
}

ApiResponse stopFailureResponse(
    const std::string& sessionId,
    const std::string& code)
{
    ApiResponse response = jsonError(503, code);
    const std::string expiredCookie =
        MediaAccessCredentialHttp::expiredSessionCookie(sessionId);
    if (!expiredCookie.empty()) {
        response.headers["Set-Cookie"] = expiredCookie;
    }
    return response;
}

ApiResponse seekedResponse(
    const StoredMediaSession& stored,
    int positionSeconds,
    int durationSeconds)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    const int position = std::max(0, positionSeconds);
    const int duration = std::max(0, durationSeconds);
    response.body =
        "{\"mediaSession\":{"
        "\"id\":\"" + jsonEscape(stored.sessionId) + "\"," +
        "\"state\":\"ready\"," +
        "\"backendId\":\"" + jsonEscape(stored.backendId) + "\"," +
        "\"recordingId\":\"" + jsonEscape(stored.resourceId) + "\"," +
        "\"presentationProfileId\":\"progressive-fmp4\"," +
        "\"growing\":false," +
        "\"mediaPath\":\"/api/media/sessions/" +
            jsonEscape(stored.sessionId) + "/recording/stream.mp4\"," +
        "\"playback\":{"
            "\"positionSeconds\":" + std::to_string(position) + "," +
            "\"durationSeconds\":" + std::to_string(duration) + "," +
            "\"seek\":{\"supported\":true,\"preparing\":false," +
                "\"window\":{\"startSeconds\":0,\"endSeconds\":" +
                    std::to_string(duration) + "}}," +
            "\"resume\":{\"supported\":true,\"preparing\":false}" +
        "}}}";
    return response;
}

} // namespace

RecordingMediaSessionController::RecordingMediaSessionController(
    VdrRecordingQueryService& recordingQueryService,
    MediaSessionRepository& mediaSessionRepository,
    MediaSessionIssuanceService& mediaSessionIssuanceService,
    std::string workspaceRoot)
    : recordingQueryService_(recordingQueryService),
      mediaSessionRepository_(mediaSessionRepository),
      mediaSessionIssuanceService_(mediaSessionIssuanceService),
      ownedDirectSourceRegistry_(std::make_unique<RecordingDirectSourceRegistry>()),
      directSourceRegistry_(ownedDirectSourceRegistry_.get()),
      mediaSessionRuntime_(std::make_unique<RecordingMediaSessionRuntime>(
          mediaSessionRepository,
          workspaceRoot,
          *directSourceRegistry_)),
      indexUpdater_(std::make_unique<VdrRecordingIndexUpdater>()),
      workspaceRoot_(std::move(workspaceRoot))
{
}

RecordingMediaSessionController::RecordingMediaSessionController(
    VdrRecordingQueryService& recordingQueryService,
    MediaSessionRepository& mediaSessionRepository,
    MediaSessionIssuanceService& mediaSessionIssuanceService,
    RecordingDirectSourceRegistry& directSourceRegistry,
    std::string workspaceRoot)
    : recordingQueryService_(recordingQueryService),
      mediaSessionRepository_(mediaSessionRepository),
      mediaSessionIssuanceService_(mediaSessionIssuanceService),
      directSourceRegistry_(&directSourceRegistry),
      mediaSessionRuntime_(std::make_unique<RecordingMediaSessionRuntime>(
          mediaSessionRepository,
          workspaceRoot,
          directSourceRegistry)),
      indexUpdater_(std::make_unique<VdrRecordingIndexUpdater>()),
      workspaceRoot_(std::move(workspaceRoot))
{
}

RecordingMediaSessionController::~RecordingMediaSessionController() = default;

std::size_t RecordingMediaSessionController::reapInactiveSessions(
    int idleTimeoutSeconds) const
{
    const std::size_t reaped =
        mediaSessionRuntime_->reapInactive(idleTimeoutSeconds);

    std::lock_guard<std::mutex> lock(selectedAudioStreamMutex_);
    for (auto it = selectedAudioStreamIndexes_.begin();
         it != selectedAudioStreamIndexes_.end();) {
        const auto stored = mediaSessionRepository_.findSession(it->first);
        if (!stored.has_value() || stored->state != "ready") {
            it = selectedAudioStreamIndexes_.erase(it);
        }
        else {
            ++it;
        }
    }
    return reaped;
}

ApiResponse RecordingMediaSessionController::handleRequest(
    const std::string& body,
    const std::string& actorId) const
{
    if (actorId.empty()) {
        return jsonError(401, "media_actor_required");
    }

    const RecordingMediaSessionTrackStatusRequest trackStatusRequest =
        RecordingMediaSessionRequestParser().parseTrackStatus(body);
    if (trackStatusRequest.valid) {
        return trackStatus(body, actorId);
    }
    if (trackStatusRequest.reasonCode != "media_session_track_status_not_requested") {
        return jsonError(
            400,
            trackStatusRequest.reasonCode.empty()
                ? "invalid_media_session_operation"
                : trackStatusRequest.reasonCode);
    }

    const RecordingMediaSessionAudioTrackSelectionRequest audioTrackRequest =
        RecordingMediaSessionRequestParser().parseAudioTrackSelection(body);
    if (audioTrackRequest.valid) {
        return selectAudioTrack(body, actorId);
    }
    if (audioTrackRequest.reasonCode !=
        "media_session_audio_track_selection_not_requested") {
        return jsonError(
            400,
            audioTrackRequest.reasonCode.empty()
                ? "invalid_media_session_operation"
                : audioTrackRequest.reasonCode);
    }

    const RecordingMediaSessionPlaybackStatusRequest statusRequest =
        RecordingMediaSessionRequestParser().parsePlaybackStatus(body);
    if (statusRequest.valid) {
        return playbackStatus(body, actorId);
    }
    if (statusRequest.reasonCode !=
        "media_session_playback_status_not_requested") {
        return jsonError(
            400,
            statusRequest.reasonCode.empty()
                ? "invalid_media_session_operation"
                : statusRequest.reasonCode);
    }

    const RecordingMediaSessionSeekRequest seekRequest =
        RecordingMediaSessionRequestParser().parseSeek(body);
    if (seekRequest.valid) {
        return seekSession(body, actorId);
    }
    if (seekRequest.reasonCode != "media_session_seek_not_requested") {
        return jsonError(
            400,
            seekRequest.reasonCode.empty()
                ? "invalid_media_session_operation"
                : seekRequest.reasonCode);
    }

    const RecordingMediaSessionStopRequest stopRequest =
        RecordingMediaSessionRequestParser().parseStop(body);
    if (stopRequest.valid) {
        return stopSession(body, actorId);
    }
    if (stopRequest.reasonCode != "media_session_stop_not_requested") {
        return jsonError(
            400,
            stopRequest.reasonCode.empty()
                ? "invalid_media_session_operation"
                : stopRequest.reasonCode);
    }

    return createSession(body, actorId);
}

ApiResponse RecordingMediaSessionController::stopSession(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionStopRequest request =
        RecordingMediaSessionRequestParser().parseStop(body);
    if (!request.valid) {
        return jsonError(
            400,
            request.reasonCode.empty()
                ? "invalid_media_session_stop_request"
                : request.reasonCode);
    }

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId) {
        return jsonError(404, "media_session_not_found");
    }
    if (stored->actorId != actorId) {
        return jsonError(403, "media_session_not_owned");
    }

    {
        std::lock_guard<std::mutex> lock(pendingIndexMutex_);
        pendingIndex_.erase(request.sessionId);
    }
    {
        std::lock_guard<std::mutex> lock(selectedAudioStreamMutex_);
        selectedAudioStreamIndexes_.erase(request.sessionId);
    }

    if (stored->state == "ended" || stored->state == "failed") {
        return stoppedResponse(request.sessionId);
    }

    if (!mediaSessionRuntime_->stop(request.sessionId, "client_closed")) {
        mediaSessionRepository_.endBundle(
            request.sessionId,
            "client_stop_runtime_unavailable");
        return stopFailureResponse(
            request.sessionId,
            "media_session_stop_failed");
    }

    return stoppedResponse(request.sessionId);
}

ApiResponse RecordingMediaSessionController::seekSession(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionSeekRequest request =
        RecordingMediaSessionRequestParser().parseSeek(body);
    if (!request.valid) {
        return jsonError(
            400,
            request.reasonCode.empty()
                ? "invalid_recording_seek_request"
                : request.reasonCode);
    }

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId) {
        return jsonError(404, "media_session_not_found");
    }
    if (stored->actorId != actorId) {
        return jsonError(403, "media_session_not_owned");
    }
    if (stored->resourceKind != "recording" ||
        stored->presentationProfileId != "progressive-fmp4") {
        return jsonError(409, "recording_seek_not_supported");
    }
    if (stored->state != "ready") {
        return jsonError(409, "media_session_not_active");
    }

    const RecordingMediaSessionSeekResult seek =
        mediaSessionRuntime_->seekStream(
            request.sessionId,
            request.positionSeconds);
    if (!seek.repositioned) {
        if (seek.reasonCode == "recording_seek_not_supported") {
            return jsonError(409, seek.reasonCode);
        }
        if (seek.reasonCode == "recording_seek_outside_window" ||
            seek.reasonCode == "invalid_recording_seek_request") {
            return jsonError(422, seek.reasonCode);
        }
        return jsonError(
            503,
            seek.reasonCode.empty()
                ? "recording_seek_failed"
                : seek.reasonCode);
    }

    return seekedResponse(*stored, seek.positionSeconds, seek.durationSeconds);
}
