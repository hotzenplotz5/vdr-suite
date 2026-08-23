#include "RecordingMediaSessionController.h"

#include "MediaSessionRepository.h"
#include "RecordingMediaSessionRequestParser.h"
#include "RecordingMediaSessionRuntime.h"
#include "VdrRecordingDuration.h"
#include "VdrRecordingIndexUpdater.h"
#include "VdrRecordingQueryService.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{

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

bool hlsProfile(const std::string& profileId)
{
    return profileId == "hls-fmp4" || profileId == "hls-ts";
}

std::string recordingMediaPath(const StoredMediaSession& stored)
{
    if (stored.presentationProfileId == "progressive-fmp4") {
        return "/api/media/sessions/" + stored.sessionId + "/recording/stream.mp4";
    }
    if (hlsProfile(stored.presentationProfileId)) {
        return "/api/media/sessions/" + stored.sessionId + "/hls/master.m3u8";
    }
    return {};
}

std::string playbackJson(
    int positionSeconds,
    int durationSeconds,
    bool seekSupported,
    bool seekPreparing,
    bool resumeSupported,
    bool resumePreparing,
    const std::string& reason = {})
{
    std::string result =
        "{\"positionSeconds\":" + std::to_string(std::max(0, positionSeconds)) +
        ",\"durationSeconds\":";
    if (durationSeconds > 0) result += std::to_string(durationSeconds);
    else result += "null";

    result += ",\"seek\":{\"supported\":";
    result += seekSupported ? "true" : "false";
    result += ",\"preparing\":";
    result += seekPreparing ? "true" : "false";
    if (seekSupported && durationSeconds > 0) {
        result +=
            ",\"window\":{\"startSeconds\":0,\"endSeconds\":" +
            std::to_string(durationSeconds) + "}";
    }
    if (!reason.empty()) {
        result += ",\"reason\":\"" + jsonEscape(reason) + "\"";
    }
    result += "},\"resume\":{\"supported\":";
    result += resumeSupported ? "true" : "false";
    result += ",\"preparing\":";
    result += resumePreparing ? "true" : "false";
    result += "}}";
    return result;
}

ApiResponse recordingPlaybackResponse(
    const StoredMediaSession& stored,
    int positionSeconds,
    int durationSeconds,
    bool seekSupported,
    bool seekPreparing,
    bool resumeSupported,
    bool resumePreparing,
    const std::string& reason = {})
{
    const std::string mediaPath = recordingMediaPath(stored);
    if (mediaPath.empty()) {
        return jsonError(409, "recording_playback_status_not_supported");
    }

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.body =
        "{\"mediaSession\":{"
        "\"id\":\"" + jsonEscape(stored.sessionId) + "\"," +
        "\"state\":\"ready\"," +
        "\"backendId\":\"" + jsonEscape(stored.backendId) + "\"," +
        "\"recordingId\":\"" + jsonEscape(stored.resourceId) + "\"," +
        "\"presentationProfileId\":\"" + jsonEscape(stored.presentationProfileId) + "\"," +
        "\"growing\":false," +
        "\"mediaPath\":\"" + jsonEscape(mediaPath) + "\"," +
        "\"playback\":" + playbackJson(
            positionSeconds,
            durationSeconds,
            seekSupported,
            seekPreparing,
            resumeSupported,
            resumePreparing,
            reason) +
        "}}";
    return response;
}

} // namespace

ApiResponse RecordingMediaSessionController::playbackStatus(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionPlaybackStatusRequest request =
        RecordingMediaSessionRequestParser().parsePlaybackStatus(body);
    if (!request.valid) {
        return jsonError(
            400,
            request.reasonCode.empty()
                ? "invalid_media_session_playback_status_request"
                : request.reasonCode);
    }

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId) {
        return jsonError(404, "media_session_not_found");
    }
    if (stored->actorId != actorId) {
        return jsonError(403, "media_session_not_owned");
    }
    const bool progressive = stored->presentationProfileId == "progressive-fmp4";
    const bool hls = hlsProfile(stored->presentationProfileId);
    if (stored->resourceKind != "recording" || (!progressive && !hls)) {
        return jsonError(409, "recording_playback_status_not_supported");
    }
    if (stored->state != "ready") {
        return jsonError(409, "media_session_not_active");
    }

    PendingIndexContext context;
    {
        std::lock_guard<std::mutex> lock(pendingIndexMutex_);
        const auto found = pendingIndex_.find(request.sessionId);
        if (found == pendingIndex_.end()) {
            return jsonError(409, "recording_index_update_not_pending");
        }
        context = found->second;
    }

    const VdrRecordingIndexUpdateResult update =
        indexUpdater_->status(context.recordingDirectory);
    if (update.running()) {
        return recordingPlaybackResponse(
            *stored,
            0,
            0,
            false,
            progressive,
            false,
            true);
    }

    if (!update.succeeded()) {
        {
            std::lock_guard<std::mutex> lock(pendingIndexMutex_);
            pendingIndex_.erase(request.sessionId);
        }
        return recordingPlaybackResponse(
            *stored,
            0,
            0,
            false,
            false,
            false,
            false,
            update.reasonCode.empty()
                ? "recording_index_update_failed"
                : update.reasonCode);
    }

    VdrRecording recording;
    if (!recordingQueryService_.findRecordingById(
            context.backendId,
            context.recordingId,
            recording)) {
        std::lock_guard<std::mutex> lock(pendingIndexMutex_);
        pendingIndex_.erase(request.sessionId);
        return recordingPlaybackResponse(
            *stored,
            0,
            0,
            false,
            false,
            false,
            false,
            "recording_index_result_unavailable");
    }

    const int durationSeconds =
        recording.recordingDurationKnown && recording.durationSeconds > 0
            ? recording.durationSeconds
            : 0;
    const std::vector<double> segmentDurations =
        durationSeconds > 0
            ? vdrsuite::recording::segmentDurationsSecondsFromIndex(
                  recording,
                  context.sourceSegments)
            : std::vector<double>{};
    if (durationSeconds <= 0 || segmentDurations.size() != context.sourceSegments.size()) {
        std::lock_guard<std::mutex> lock(pendingIndexMutex_);
        pendingIndex_.erase(request.sessionId);
        return recordingPlaybackResponse(
            *stored,
            0,
            durationSeconds,
            false,
            false,
            false,
            false,
            "recording_index_timeline_unavailable");
    }

    recordingQueryService_.updateCachedRecording(recording);

    if (hls) {
        std::lock_guard<std::mutex> lock(pendingIndexMutex_);
        pendingIndex_.erase(request.sessionId);
        return recordingPlaybackResponse(
            *stored,
            0,
            durationSeconds,
            false,
            false,
            true,
            false);
    }

    const RecordingMediaSessionSeekCapabilityResult enabled =
        mediaSessionRuntime_->enableIndexedSeek(
            request.sessionId,
            durationSeconds,
            context.sourceSegments,
            segmentDurations);
    if (!enabled.enabled) {
        std::lock_guard<std::mutex> lock(pendingIndexMutex_);
        pendingIndex_.erase(request.sessionId);
        return recordingPlaybackResponse(
            *stored,
            0,
            durationSeconds,
            false,
            false,
            false,
            false,
            enabled.reasonCode.empty()
                ? "recording_seek_timeline_activation_failed"
                : enabled.reasonCode);
    }

    {
        std::lock_guard<std::mutex> lock(pendingIndexMutex_);
        pendingIndex_.erase(request.sessionId);
    }
    return recordingPlaybackResponse(
        *stored,
        0,
        enabled.durationSeconds,
        true,
        false,
        true,
        false);
}
