#include "RecordingMediaSessionController.h"

#include "LocalVdrRecordingSourceResolver.h"
#include "MediaPresentationSelector.h"
#include "MediaSessionRepository.h"
#include "MediaTranscodeSettingsApiRuntime.h"
#include "RecordingMediaSessionRequestParser.h"
#include "RecordingMediaSessionRuntime.h"
#include "RecordingMediaTrackContract.h"
#include "RecordingSubtitleSidecar.h"
#include "VdrRecordingDuration.h"
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
        if (character == '"') result += "\\\"";
        else if (character == '\\') result += "\\\\";
        else if (character >= 0x20U) result.push_back(static_cast<char>(character));
    }
    return result;
}

std::string descriptorCacheKey(const std::string& backendId, const std::string& recordingId)
{
    return backendId + "\n" + recordingId;
}

bool hlsProfile(const std::string& profileId)
{
    return profileId == "hls-fmp4" || profileId == "hls-ts";
}

bool subtitleAdaptedProfile(const std::string& profileId)
{
    return profileId == "progressive-fmp4" || hlsProfile(profileId);
}

std::size_t selectableSubtitleCount(const MediaSourceDescriptor& source)
{
    return static_cast<std::size_t>(std::count_if(
        source.subtitleStreams.begin(),
        source.subtitleStreams.end(),
        [](const MediaSubtitleStreamDescriptor& track) {
            return RecordingMediaTrackContract::subtitleTrackSelectable(track.format);
        }));
}

std::string subtitleSelectionReason(
    const MediaSourceDescriptor& source,
    bool adaptedProfile)
{
    if (source.subtitleStreams.empty()) return "no_subtitle_tracks";
    if (selectableSubtitleCount(source) == 0) return "no_browser_text_subtitle_tracks";
    if (!adaptedProfile) return "profile_does_not_deliver_selectable_subtitles";
    return {};
}

bool appendRecordingSubtitleSidecar(
    VdrRecordingQueryService& recordingQueryService,
    const StoredMediaSession& stored,
    MediaSourceDescriptor& source)
{
    VdrRecording recording;
    if (!recordingQueryService.findRecordingById(
            stored.backendId,
            stored.resourceId,
            recording)) {
        return false;
    }

    LocalVdrRecordingSourceResolver trustedResolver(
        [recording](const std::string& backendId) {
            if (recording.backendId != backendId &&
                !(recording.backendId.empty() && backendId == "default")) {
                return std::vector<VdrRecording>{};
            }
            return std::vector<VdrRecording>{recording};
        });
    const LocalVdrRecordingSourceResolution resolved =
        trustedResolver.resolve(stored.backendId, stored.resourceId);
    if (!resolved.resolved || resolved.source.growing) return false;

    return RecordingSubtitleSidecar::appendTo(
        source,
        resolved.source.recordingDirectory);
}

std::string transcodePolicyReasonCode(const MediaPresentationProfile& profile)
{
    if (profile.reason.find("forced VAAPI does not support") != std::string::npos)
        return "forced_vaapi_transformation_unsupported";
    if (profile.reason.find("forced VAAPI is unavailable") != std::string::npos)
        return "forced_vaapi_unavailable";
    return "media_transcode_capacity_unproven";
}

ApiResponse trackResponse(const StoredMediaSession& stored, const std::string& tracksJson)
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
        "\"presentationProfileId\":\"" + jsonEscape(stored.presentationProfileId) + "\"," +
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
        "\"mediaPath\":\"/api/media/sessions/" + jsonEscape(stored.sessionId) + "/recording/stream.mp4\"," +
        "\"playback\":{" 
            "\"positionSeconds\":" + std::to_string(position) + "," +
            "\"durationSeconds\":" + std::to_string(duration) + "," +
            "\"seek\":{\"supported\":true,\"preparing\":false," +
                "\"window\":{\"startSeconds\":0,\"endSeconds\":" + std::to_string(duration) + "}}," +
            "\"resume\":{\"supported\":true,\"preparing\":false}}," +
        "\"tracks\":" + tracksJson + "}}";
    return response;
}

ApiResponse subtitleOffResponse(const StoredMediaSession& stored)
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
        "\"subtitleTrackId\":\"off\"}}";
    return response;
}

ApiResponse subtitleWebVttResponse(
    const RecordingMediaSessionSubtitleWebVttResult& subtitle)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "text/vtt; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["Cross-Origin-Resource-Policy"] = "same-origin";
    response.body = subtitle.webVtt;
    return response;
}

bool hlsRestartTimelineReady(
    VdrRecordingQueryService& recordingQueryService,
    const StoredMediaSession& stored)
{
    VdrRecording recording;
    if (!recordingQueryService.findRecordingById(stored.backendId, stored.resourceId, recording))
        return false;

    LocalVdrRecordingSourceResolver trustedResolver(
        [recording](const std::string& backendId) {
            if (recording.backendId != backendId &&
                !(recording.backendId.empty() && backendId == "default"))
                return std::vector<VdrRecording>{};
            return std::vector<VdrRecording>{recording};
        });
    const LocalVdrRecordingSourceResolution sourceResolution =
        trustedResolver.resolve(stored.backendId, stored.resourceId);
    if (!sourceResolution.resolved || sourceResolution.source.growing ||
        !recording.recordingDurationKnown || recording.durationSeconds <= 0 ||
        sourceResolution.source.segmentPaths.empty())
        return false;

    const std::vector<double> segmentDurations =
        vdrsuite::recording::segmentDurationsSecondsFromIndex(
            recording,
            sourceResolution.source.segmentPaths);
    return !segmentDurations.empty() &&
        segmentDurations.size() == sourceResolution.source.segmentPaths.size();
}

} // namespace

ApiResponse RecordingMediaSessionController::trackStatus(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionTrackStatusRequest request =
        RecordingMediaSessionRequestParser().parseTrackStatus(body);
    if (!request.valid)
        return jsonError(400, request.reasonCode.empty()
            ? "invalid_recording_track_status_request" : request.reasonCode);

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId)
        return jsonError(404, "media_session_not_found");
    if (stored->actorId != actorId) return jsonError(403, "media_session_not_owned");
    if (stored->resourceKind != "recording")
        return jsonError(409, "recording_track_status_not_supported");
    if (stored->state != "ready") return jsonError(409, "media_session_not_active");

    MediaSourceDescriptor source;
    {
        std::lock_guard<std::mutex> lock(descriptorCacheMutex_);
        const auto found = descriptorCache_.find(descriptorCacheKey(stored->backendId, stored->resourceId));
        if (found == descriptorCache_.end())
            return jsonError(409, "recording_track_metadata_unavailable");
        source = found->second.source;
    }
    appendRecordingSubtitleSidecar(recordingQueryService_, *stored, source);

    const RecordingMediaSessionTrackState state = mediaSessionRuntime_->trackState(request.sessionId);
    int selectedAudioStreamIndex = -1;
    {
        std::lock_guard<std::mutex> lock(selectedAudioStreamMutex_);
        const auto selected = selectedAudioStreamIndexes_.find(request.sessionId);
        if (selected != selectedAudioStreamIndexes_.end()) selectedAudioStreamIndex = selected->second;
    }
    if (selectedAudioStreamIndex < 0 && state.available &&
        state.profileId == stored->presentationProfileId)
        selectedAudioStreamIndex = state.sourceAudioStreamIndex;

    int selectedSubtitleStreamIndex = -1;
    {
        std::lock_guard<std::mutex> lock(selectedSubtitleStreamMutex_);
        const auto selected = selectedSubtitleStreamIndexes_.find(request.sessionId);
        if (selected != selectedSubtitleStreamIndexes_.end()) selectedSubtitleStreamIndex = selected->second;
    }

    bool audioSelectionSupported = false;
    std::string audioSelectionReason;
    bool preparing = false;
    {
        std::lock_guard<std::mutex> lock(pendingIndexMutex_);
        preparing = pendingIndex_.find(request.sessionId) != pendingIndex_.end();
    }

    if (stored->presentationProfileId == "progressive-fmp4") {
        if (state.available && state.audioSelectionSupported) audioSelectionSupported = true;
        else audioSelectionReason = preparing
            ? "recording_audio_track_selection_preparing"
            : (state.available
                ? "recording_audio_track_selection_timeline_unavailable"
                : "recording_audio_track_runtime_unavailable");
    }
    else if (hlsProfile(stored->presentationProfileId)) {
        if (preparing) audioSelectionReason = "recording_audio_track_selection_preparing";
        else if (hlsRestartTimelineReady(recordingQueryService_, *stored)) audioSelectionSupported = true;
        else audioSelectionReason = "recording_audio_track_selection_timeline_unavailable";
    }
    else audioSelectionReason = "recording_audio_track_selection_profile_not_supported";

    const bool adaptedProfile = subtitleAdaptedProfile(stored->presentationProfileId);
    const std::string subtitleReason = subtitleSelectionReason(source, adaptedProfile);
    const bool subtitleSelectionSupported = adaptedProfile &&
        selectableSubtitleCount(source) > 0;
    const std::string tracks = RecordingMediaTrackContract::json(
        source,
        selectedAudioStreamIndex,
        audioSelectionSupported,
        audioSelectionReason,
        subtitleSelectionSupported,
        subtitleReason,
        adaptedProfile,
        adaptedProfile ? (selectedSubtitleStreamIndex < 0 ? 1 : 0) : -1,
        selectedSubtitleStreamIndex);
    return trackResponse(*stored, tracks);
}

ApiResponse RecordingMediaSessionController::selectAudioTrack(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionAudioTrackSelectionRequest request =
        RecordingMediaSessionRequestParser().parseAudioTrackSelection(body);
    if (!request.valid)
        return jsonError(400, request.reasonCode.empty()
            ? "invalid_recording_audio_track_selection_request" : request.reasonCode);

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId ||
        stored->resourceId != request.recordingId)
        return jsonError(404, "media_session_not_found");
    if (stored->actorId != actorId) return jsonError(403, "media_session_not_owned");
    if (stored->resourceKind != "recording" || stored->presentationProfileId != "progressive-fmp4")
        return jsonError(409, "recording_audio_track_selection_not_supported");
    if (stored->state != "ready") return jsonError(409, "media_session_not_active");

    MediaSourceDescriptor source;
    {
        std::lock_guard<std::mutex> lock(descriptorCacheMutex_);
        const auto found = descriptorCache_.find(descriptorCacheKey(stored->backendId, stored->resourceId));
        if (found == descriptorCache_.end())
            return jsonError(409, "recording_track_metadata_unavailable");
        source = found->second.source;
    }
    appendRecordingSubtitleSidecar(recordingQueryService_, *stored, source);

    int sourceAudioStreamIndex = -1;
    if (!RecordingMediaTrackContract::audioStreamIndexForTrackId(
            request.audioTrackId, source, sourceAudioStreamIndex))
        return jsonError(404, "recording_audio_track_not_found");

    const RecordingMediaSessionTrackState state = mediaSessionRuntime_->trackState(request.sessionId);
    if (!state.available) return jsonError(503, "recording_audio_track_runtime_unavailable");
    if (!state.audioSelectionSupported) {
        bool selectionPreparing = false;
        {
            std::lock_guard<std::mutex> lock(pendingIndexMutex_);
            selectionPreparing = pendingIndex_.find(request.sessionId) != pendingIndex_.end();
        }
        return jsonError(409, selectionPreparing
            ? "recording_audio_track_selection_preparing"
            : "recording_audio_track_selection_not_supported");
    }

    MediaPresentationProfile profile = MediaPresentationSelector().select(
        source, request.capabilities, sourceAudioStreamIndex);
    if (!profile.available || profile.profileId != "progressive-fmp4")
        return jsonError(409, "recording_audio_track_selection_unsupported");
    if (profile.videoAction == MediaTrackAction::Transcode) {
        const MediaTranscodePolicy policy =
            MediaTranscodeSettingsApiRuntime::instance().resolvePolicy(request.backendId);
        profile = policy.apply(profile);
        if (!profile.available) return jsonError(503, transcodePolicyReasonCode(profile));
    }

    const RecordingMediaSessionAudioTrackSelectionResult selected =
        mediaSessionRuntime_->selectAudioTrack(request.sessionId, profile, request.positionSeconds);
    if (!selected.selected) {
        if (selected.reasonCode == "recording_audio_track_selection_not_supported" ||
            selected.reasonCode == "recording_audio_track_selection_video_change_not_allowed")
            return jsonError(409, selected.reasonCode);
        if (selected.reasonCode == "recording_audio_track_position_outside_window" ||
            selected.reasonCode == "invalid_recording_audio_track_selection")
            return jsonError(422, selected.reasonCode);
        return jsonError(503, selected.reasonCode.empty()
            ? "recording_audio_track_restart_failed" : selected.reasonCode);
    }

    {
        std::lock_guard<std::mutex> lock(selectedAudioStreamMutex_);
        selectedAudioStreamIndexes_[request.sessionId] = selected.sourceAudioStreamIndex;
    }
    int selectedSubtitleStreamIndex = -1;
    {
        std::lock_guard<std::mutex> lock(selectedSubtitleStreamMutex_);
        const auto subtitle = selectedSubtitleStreamIndexes_.find(request.sessionId);
        if (subtitle != selectedSubtitleStreamIndexes_.end()) selectedSubtitleStreamIndex = subtitle->second;
    }
    const bool subtitleSupported = selectableSubtitleCount(source) > 0;
    const std::string tracks = RecordingMediaTrackContract::json(
        source,
        selected.sourceAudioStreamIndex,
        true,
        "",
        subtitleSupported,
        subtitleSelectionReason(source, true),
        true,
        selectedSubtitleStreamIndex < 0 ? 1 : 0,
        selectedSubtitleStreamIndex);
    return selectedResponse(*stored, selected, tracks);
}

ApiResponse RecordingMediaSessionController::selectSubtitleTrack(
    const std::string& body,
    const std::string& actorId) const
{
    const RecordingMediaSessionSubtitleTrackSelectionRequest request =
        RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(body);
    if (!request.valid)
        return jsonError(400, request.reasonCode.empty()
            ? "invalid_recording_subtitle_track_selection_request" : request.reasonCode);

    const auto stored = mediaSessionRepository_.findSession(request.sessionId);
    if (!stored.has_value() || stored->backendId != request.backendId)
        return jsonError(404, "media_session_not_found");
    if (stored->actorId != actorId) return jsonError(403, "media_session_not_owned");
    if (stored->resourceKind != "recording" ||
        !subtitleAdaptedProfile(stored->presentationProfileId))
        return jsonError(409, "recording_subtitle_track_selection_not_supported");
    if (stored->state != "ready") return jsonError(409, "media_session_not_active");

    MediaSourceDescriptor source;
    {
        std::lock_guard<std::mutex> lock(descriptorCacheMutex_);
        const auto found = descriptorCache_.find(descriptorCacheKey(stored->backendId, stored->resourceId));
        if (found == descriptorCache_.end())
            return jsonError(409, "recording_track_metadata_unavailable");
        source = found->second.source;
    }
    appendRecordingSubtitleSidecar(recordingQueryService_, *stored, source);

    if (request.subtitleTrackId == "off") {
        std::lock_guard<std::mutex> lock(selectedSubtitleStreamMutex_);
        selectedSubtitleStreamIndexes_.erase(request.sessionId);
        return subtitleOffResponse(*stored);
    }

    int sourceSubtitleStreamIndex = -1;
    if (!RecordingMediaTrackContract::subtitleStreamIndexForTrackId(
            request.subtitleTrackId, source, sourceSubtitleStreamIndex))
        return jsonError(404, "recording_subtitle_track_not_found");
    if (sourceSubtitleStreamIndex < 0 ||
        static_cast<std::size_t>(sourceSubtitleStreamIndex) >= source.subtitleStreams.size())
        return jsonError(404, "recording_subtitle_track_not_found");

    const MediaSubtitleStreamDescriptor& subtitleTrack =
        source.subtitleStreams[static_cast<std::size_t>(sourceSubtitleStreamIndex)];
    if (!RecordingMediaTrackContract::subtitleTrackSelectable(subtitleTrack.format))
        return jsonError(409, "recording_subtitle_track_not_browser_text");

    const RecordingMediaSessionSubtitleWebVttResult subtitle =
        mediaSessionRuntime_->subtitleWebVtt(
            request.sessionId,
            sourceSubtitleStreamIndex,
            subtitleTrack.format,
            request.streamBasePositionSeconds,
            subtitleTrack.externalSourcePath);
    if (!subtitle.ready) {
        if (subtitle.reasonCode == "invalid_recording_subtitle_stream_index" ||
            subtitle.reasonCode == "invalid_recording_subtitle_stream_base" ||
            subtitle.reasonCode == "invalid_recording_subtitle_external_source")
            return jsonError(422, subtitle.reasonCode);
        if (subtitle.reasonCode == "recording_subtitle_format_not_webvtt_convertible" ||
            subtitle.reasonCode == "recording_subtitle_external_format_not_supported" ||
            subtitle.reasonCode == "recording_subtitle_delivery_not_supported")
            return jsonError(409, subtitle.reasonCode);
        return jsonError(503, subtitle.reasonCode.empty()
            ? "recording_subtitle_extract_failed" : subtitle.reasonCode);
    }

    {
        std::lock_guard<std::mutex> lock(selectedSubtitleStreamMutex_);
        selectedSubtitleStreamIndexes_[request.sessionId] = subtitle.sourceSubtitleStreamIndex;
    }
    return subtitleWebVttResponse(subtitle);
}
