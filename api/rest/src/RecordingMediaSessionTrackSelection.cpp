#include "RecordingMediaSessionController.h"

#include "MediaPresentationSelector.h"
#include "MediaSessionRepository.h"
#include "MediaTranscodeSettingsApiRuntime.h"
#include "RecordingMediaSessionRequestParser.h"
#include "RecordingMediaSessionRuntime.h"
#include "RecordingMediaTrackContract.h"

#include <algorithm>
#include <string>

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
        if (character == '"') result += "\\\"";
        else if (character == '\\') result += "\\\\";
        else if (character >= 0x20U) result.push_back(static_cast<char>(character));
    }
    return result;
}

std::string descriptorCacheKey(
    const std::string& backendId,
    const std::string& recordingId)
{
    return backendId + "\n" + recordingId;
}

bool hlsProfile(const std::string& profileId)
{
    return profileId == "hls-fmp4" || profileId == "hls-ts";
}

std::string transcodePolicyReasonCode(const MediaPresentationProfile& profile)
{
    if (profile.reason.find("forced VAAPI does not support") != std::string::npos) {
        return "forced_vaapi_transformation_unsupported";
    }
    if (profile.reason.find("forced VAAPI is unavailable") != std::string::npos) {
        return "forced_vaapi_unavailable";
    }
    return "media_transcode_capacity_unproven";
}

ApiResponse trackResponse(
    const StoredMediaSession& stored,
    const std::string& tracksJson)
{
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
        "\"presentationProfileId\":\"" +
            jsonEscape(stored.presentationProfileId) + "\"," +
        "\"tracks\":" + tracksJson + "}}";
    return response;
}

ApiResponse selectedResponse(
    const StoredMediaSession& stored,
    const RecordingMediaSessionAudioTrackSelectionResult& selected,
    const std::string& tracksJson)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    const int position = std::max(0, selected.positionSeconds);
    const int duration = std::max(0, selected.durationSeconds);
    response.body =
        "{\"mediaSession\":{"
        "\"id\":\"" + jsonEscape(stored.sessionId) + "\"," +
        "\"state\":\"ready\"," +
        "\"backendId\":\"" + jsonEscape(stored.backendId) + "\"," +
        "\"recordingId\":\"" + jsonEscape(stored.resourceId) + "\"," +
        "\"presentationProfileId\":\"progressive-fmp4\"," +
        "\"mediaPath\":\"/api/media/sessions/" +
            jsonEscape(stored.sessionId) + "/recording/stream.mp4\"," +
        "\"playback\":{"
            "\"positionSeconds\":" + std::to_string(position) + "," +
            "\"durationSeconds\":" + std::to_string(duration) + "," +
            "\"seek\":{\"supported\":true,\"preparing\":false," +
                "\"window\":{\"startSeconds\":0,\"endSeconds\":" +
                    std::to_string(duration) + "}}," +
            "\"resume\":{\"supported\":true,\"preparing\":false}}," +
        "\"tracks\":" + tracksJson + "}}";
    return response;
}

} // namespace

ApiResponse RecordingMediaSessionController::trackStatus(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionTrackStatusRequest request =
        RecordingMediaSessionRequestParser().parseTrackStatus(body);
    if (!request.valid) {
        return jsonError(400, request.reasonCode.empty()
            ? "invalid_recording_track_status_request" : request.reasonCode);
    }

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId) {
        return jsonError(404, "media_session_not_found");
    }
    if (stored->actorId != actorId) return jsonError(403, "media_session_not_owned");
    if (stored->resourceKind != "recording") {
        return jsonError(409, "recording_track_status_not_supported");
    }
    if (stored->state != "ready") return jsonError(409, "media_session_not_active");

    MediaSourceDescriptor source;
    {
        std::lock_guard<std::mutex> lock(descriptorCacheMutex_);
        const auto found = descriptorCache_.find(
            descriptorCacheKey(stored->backendId, stored->resourceId));
        if (found == descriptorCache_.end()) {
            return jsonError(409, "recording_track_metadata_unavailable");
        }
        source = found->second.source;
    }

    const RecordingMediaSessionTrackState state =
        mediaSessionRuntime_->trackState(request.sessionId);
    int selectedAudioStreamIndex = -1;
    bool audioSelectionSupported = false;
    std::string audioSelectionReason;

    if (stored->presentationProfileId == "progressive-fmp4") {
        if (state.available) selectedAudioStreamIndex = state.sourceAudioStreamIndex;
        if (state.available && state.audioSelectionSupported) {
            audioSelectionSupported = true;
        }
        else {
            bool preparing = false;
            {
                std::lock_guard<std::mutex> lock(pendingIndexMutex_);
                preparing = pendingIndex_.find(request.sessionId) != pendingIndex_.end();
            }
            audioSelectionReason = preparing
                ? "recording_audio_track_selection_preparing"
                : (state.available
                    ? "recording_audio_track_selection_timeline_unavailable"
                    : "recording_audio_track_runtime_unavailable");
        }
    }
    else {
        if (state.available && state.profileId == stored->presentationProfileId) {
            selectedAudioStreamIndex = state.sourceAudioStreamIndex;
        }
        audioSelectionReason = "recording_audio_track_selection_profile_not_supported";
    }

    const bool adaptedProfile =
        stored->presentationProfileId == "progressive-fmp4" ||
        hlsProfile(stored->presentationProfileId);
    const std::string subtitleReason = source.subtitleStreams.empty()
        ? "no_subtitle_tracks"
        : "profile_does_not_deliver_selectable_subtitles";
    const std::string tracks = RecordingMediaTrackContract::json(
        source,
        selectedAudioStreamIndex,
        audioSelectionSupported,
        audioSelectionReason,
        false,
        subtitleReason,
        adaptedProfile,
        adaptedProfile ? 1 : -1);
    return trackResponse(*stored, tracks);
}

ApiResponse RecordingMediaSessionController::selectAudioTrack(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionAudioTrackSelectionRequest request =
        RecordingMediaSessionRequestParser().parseAudioTrackSelection(body);
    if (!request.valid) {
        return jsonError(400, request.reasonCode.empty()
            ? "invalid_recording_audio_track_selection_request" : request.reasonCode);
    }

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId ||
        stored->resourceId != request.recordingId) {
        return jsonError(404, "media_session_not_found");
    }
    if (stored->actorId != actorId) return jsonError(403, "media_session_not_owned");
    if (stored->resourceKind != "recording" ||
        stored->presentationProfileId != "progressive-fmp4") {
        return jsonError(409, "recording_audio_track_selection_not_supported");
    }
    if (stored->state != "ready") return jsonError(409, "media_session_not_active");

    MediaSourceDescriptor source;
    {
        std::lock_guard<std::mutex> lock(descriptorCacheMutex_);
        const auto found = descriptorCache_.find(
            descriptorCacheKey(stored->backendId, stored->resourceId));
        if (found == descriptorCache_.end()) {
            return jsonError(409, "recording_track_metadata_unavailable");
        }
        source = found->second.source;
    }

    int sourceAudioStreamIndex = -1;
    if (!RecordingMediaTrackContract::audioStreamIndexForTrackId(
            request.audioTrackId, source, sourceAudioStreamIndex)) {
        return jsonError(404, "recording_audio_track_not_found");
    }

    const RecordingMediaSessionTrackState state =
        mediaSessionRuntime_->trackState(request.sessionId);
    if (!state.available) return jsonError(503, "recording_audio_track_runtime_unavailable");
    if (!state.audioSelectionSupported) {
        bool preparing = false;
        {
            std::lock_guard<std::mutex> lock(pendingIndexMutex_);
            preparing = pendingIndex_.find(request.sessionId) != pendingIndex_.end();
        }
        return jsonError(409, preparing
            ? "recording_audio_track_selection_preparing"
            : "recording_audio_track_selection_not_supported");
    }

    MediaPresentationProfile profile = MediaPresentationSelector().select(
        source,
        request.capabilities,
        sourceAudioStreamIndex);
    if (!profile.available || profile.profileId != "progressive-fmp4") {
        return jsonError(409, "recording_audio_track_selection_unsupported");
    }
    if (profile.videoAction == MediaTrackAction::Transcode) {
        const MediaTranscodePolicy policy =
            MediaTranscodeSettingsApiRuntime::instance().resolvePolicy(request.backendId);
        profile = policy.apply(profile);
        if (!profile.available) {
            return jsonError(503, transcodePolicyReasonCode(profile));
        }
    }

    const RecordingMediaSessionAudioTrackSelectionResult selected =
        mediaSessionRuntime_->selectAudioTrack(
            request.sessionId,
            profile,
            request.positionSeconds);
    if (!selected.selected) {
        if (selected.reasonCode == "recording_audio_track_selection_not_supported" ||
            selected.reasonCode == "recording_audio_track_selection_video_change_not_allowed") {
            return jsonError(409, selected.reasonCode);
        }
        if (selected.reasonCode == "recording_audio_track_position_outside_window" ||
            selected.reasonCode == "invalid_recording_audio_track_selection") {
            return jsonError(422, selected.reasonCode);
        }
        return jsonError(503, selected.reasonCode.empty()
            ? "recording_audio_track_restart_failed" : selected.reasonCode);
    }

    const std::string subtitleReason = source.subtitleStreams.empty()
        ? "no_subtitle_tracks"
        : "profile_does_not_deliver_selectable_subtitles";
    const std::string tracks = RecordingMediaTrackContract::json(
        source,
        selected.sourceAudioStreamIndex,
        true,
        "",
        false,
        subtitleReason,
        true,
        1);
    return selectedResponse(*stored, selected, tracks);
}
