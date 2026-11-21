#include "MediaPlaybackContract.h"

#include <algorithm>
#include <initializer_list>

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
            if (character < 0x20U) {
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

std::string nullableString(const std::string& value)
{
    return value.empty() ? "null" : "\"" + jsonEscape(value) + "\"";
}

const char* resourceModeName(MediaPlaybackResourceMode mode)
{
    switch (mode) {
    case MediaPlaybackResourceMode::Recording: return "recording";
    case MediaPlaybackResourceMode::GrowingRecording: return "growing-recording";
    case MediaPlaybackResourceMode::Live: return "live";
    }
    return "recording";
}

const char* seekModeName(MediaPlaybackSeekMode mode)
{
    switch (mode) {
    case MediaPlaybackSeekMode::Unsupported: return "unsupported";
    case MediaPlaybackSeekMode::InSessionReposition: return "in-session-reposition";
    case MediaPlaybackSeekMode::ReplacementSessionRestart: return "replacement-session-restart";
    }
    return "unsupported";
}

bool hlsProfile(const std::string& profileId)
{
    return profileId == "hls-fmp4" || profileId == "hls-ts";
}

std::string optionalBoolJson(const std::optional<bool>& value)
{
    if (!value.has_value()) return "null";
    return *value ? "true" : "false";
}

std::string optionalIntJson(const std::optional<int>& value)
{
    if (!value.has_value()) return "null";
    return std::to_string(std::max(0, *value));
}

std::string optionalStringJson(const std::optional<std::string>& value)
{
    if (!value.has_value()) return "null";
    return nullableString(*value);
}

bool optionalTrue(const std::optional<bool>& value)
{
    return value.has_value() && *value;
}

bool reasonIs(
    const std::string& reasonCode,
    std::initializer_list<const char*> candidates)
{
    for (const char* candidate : candidates) {
        if (reasonCode == candidate) return true;
    }
    return false;
}

MediaPlaybackFailureContract failure(
    const std::string& category,
    const std::string& origin,
    const std::string& stage,
    bool terminal,
    const std::string& recoveryClass,
    const std::string& reasonCode)
{
    MediaPlaybackFailureContract result;
    result.category = category;
    result.origin = origin;
    result.stage = stage;
    result.terminal = terminal;
    result.recoveryClass = recoveryClass;
    result.reasonCode = reasonCode;
    return result;
}

} // namespace

MediaPlaybackContract MediaPlaybackContractFactory::recording(
    const std::string& presentationProfileId,
    bool growing,
    int positionSeconds,
    int durationSeconds,
    int presentationBasePositionSeconds,
    bool indexedTimelineReady,
    bool indexPreparing,
    const MediaPlaybackTrackCapabilities& tracks)
{
    const bool completedTimelineReady =
        !growing && durationSeconds > 0 && indexedTimelineReady;
    const bool progressiveFmp4 = presentationProfileId == "progressive-fmp4";
    const bool hls = hlsProfile(presentationProfileId);
    const bool restartProfile = progressiveFmp4 || hls;
    const bool restartSupported = restartProfile && completedTimelineReady;
    const bool restartPreparing =
        restartProfile && !growing && !completedTimelineReady && indexPreparing;
    const bool legacySeekSupported = progressiveFmp4 && completedTimelineReady;
    const bool legacySeekPreparing =
        progressiveFmp4 && !growing && !completedTimelineReady && indexPreparing;

    return recordingFromLegacy(
        presentationProfileId,
        growing,
        positionSeconds,
        growing || durationSeconds <= 0
            ? std::optional<int>{}
            : std::optional<int>{durationSeconds},
        presentationBasePositionSeconds,
        legacySeekSupported,
        legacySeekPreparing,
        restartSupported,
        restartPreparing,
        tracks);
}

MediaPlaybackContract MediaPlaybackContractFactory::recordingFromLegacy(
    const std::string& presentationProfileId,
    std::optional<bool> growing,
    std::optional<int> positionSeconds,
    std::optional<int> durationSeconds,
    std::optional<int> presentationBasePositionSeconds,
    std::optional<bool> legacySeekSupported,
    std::optional<bool> legacySeekPreparing,
    std::optional<bool> restartSupported,
    std::optional<bool> restartPreparing,
    const MediaPlaybackTrackCapabilities& tracks)
{
    MediaPlaybackContract contract;
    contract.resourceMode = growing.has_value() && *growing
        ? MediaPlaybackResourceMode::GrowingRecording
        : MediaPlaybackResourceMode::Recording;
    contract.presentationProfileId = presentationProfileId;
    contract.positionSeconds = positionSeconds;
    contract.durationSeconds = growing.has_value() && *growing
        ? std::optional<int>{}
        : durationSeconds;
    contract.presentationBasePositionSeconds = presentationBasePositionSeconds;
    contract.pauseSupported = true;
    contract.resumePlaybackSupported = true;
    contract.restartSupported = restartSupported;
    contract.restartPreparing = restartPreparing;
    contract.tracks = tracks;

    if (presentationProfileId == "progressive-fmp4") {
        contract.seek.mode = MediaPlaybackSeekMode::InSessionReposition;
        contract.seek.supported = legacySeekSupported;
        contract.seek.preparing = legacySeekPreparing;
    }
    else if (hlsProfile(presentationProfileId)) {
        contract.seek.mode = MediaPlaybackSeekMode::ReplacementSessionRestart;
        contract.seek.supported = restartSupported;
        contract.seek.preparing = restartPreparing;
    }
    else {
        contract.seek.mode = MediaPlaybackSeekMode::Unsupported;
        contract.seek.supported = false;
        contract.seek.preparing = false;
    }

    if (optionalTrue(contract.seek.supported) &&
        contract.durationSeconds.has_value() && *contract.durationSeconds > 0) {
        contract.seek.windowStartSeconds = 0;
        contract.seek.windowEndSeconds = *contract.durationSeconds;
    }

    return contract;
}

MediaPlaybackContract MediaPlaybackContractFactory::live(
    const std::string& presentationProfileId,
    const MediaPlaybackTrackCapabilities& tracks)
{
    MediaPlaybackContract contract;
    contract.resourceMode = MediaPlaybackResourceMode::Live;
    contract.presentationProfileId = presentationProfileId;
    contract.seek.mode = MediaPlaybackSeekMode::Unsupported;
    contract.seek.supported = false;
    contract.seek.preparing = false;
    contract.tracks = tracks;
    return contract;
}

std::optional<MediaPlaybackFailureContract>
MediaPlaybackContractFactory::classifyFailure(const std::string& reasonCode)
{
    if (reasonCode.empty()) return std::nullopt;

    if (reasonIs(reasonCode, {
            "media_access_credential_required",
            "invalid_media_access_credential",
            "media_access_denied",
            "media_access_inactive",
            "media_access_fence_mismatch",
            "media_access_state_unavailable"})) {
        return failure(
            "authorization",
            "gateway",
            "access-authorization",
            true,
            "new-authorization",
            reasonCode);
    }

    if (reasonIs(reasonCode, {
            "media_actor_required",
            "media_session_not_owned"})) {
        return failure(
            "authorization",
            "control-plane",
            "session-authorization",
            true,
            "new-authorization",
            reasonCode);
    }

    if (reasonIs(reasonCode, {
            "recording_not_found",
            "recording_source_unavailable"})) {
        return failure(
            "source",
            "control-plane",
            "source-resolution",
            true,
            "none",
            reasonCode);
    }

    if (reasonIs(reasonCode, {
            "media_probe_workspace_unavailable",
            "media_source_probe_failed",
            "media_source_unsupported"})) {
        return failure(
            "source",
            "control-plane",
            "source-probe",
            true,
            "none",
            reasonCode);
    }

    if (reasonCode == "recording_resume_growing_not_supported") {
        return failure(
            "source",
            "control-plane",
            "exact-resume",
            true,
            "none",
            reasonCode);
    }

    if (reasonIs(reasonCode, {
            "media_presentation_unavailable",
            "recording_audio_track_selection_unsupported",
            "recording_resume_profile_not_supported",
            "recording_resume_sync_profile_unavailable",
            "forced_vaapi_transformation_unsupported",
            "forced_vaapi_unavailable",
            "media_transcode_capacity_unproven"})) {
        return failure(
            "adaptation",
            "control-plane",
            "presentation-selection",
            true,
            "new-authorized-contract",
            reasonCode);
    }

    if (reasonIs(reasonCode, {
            "media_worker_plan_invalid",
            "selected_source_track_missing",
            "unsupported_recording_video_transformation",
            "unsupported_recording_audio_transformation",
            "profile_is_not_recording_progressive_fmp4"})) {
        return failure(
            "adaptation",
            "media-worker",
            "provision-plan",
            true,
            "new-authorized-contract",
            reasonCode);
    }

    if (reasonIs(reasonCode, {
            "media_worker_start_failed",
            "media_worker_exited_before_ready",
            "media_worker_wait_failed",
            "media_hls_not_ready",
            "recording_stream_pipe_create_failed",
            "media_provision_failed"})) {
        return failure(
            "transport",
            "media-worker",
            "provision-start",
            true,
            "new-authorized-contract",
            reasonCode);
    }

    if (reasonIs(reasonCode, {
            "recording_resume_not_ready",
            "recording_resume_outside_window"})) {
        return failure(
            "timeline",
            "control-plane",
            "exact-resume",
            true,
            "none",
            reasonCode);
    }

    if (reasonCode == "recording_seek_outside_window") {
        return failure(
            "timeline",
            "control-plane",
            "seek",
            false,
            "none",
            reasonCode);
    }

    if (reasonIs(reasonCode, {
            "recording_index_update_failed",
            "recording_index_result_unavailable",
            "recording_index_timeline_unavailable",
            "recording_seek_timeline_activation_failed"})) {
        return failure(
            "timeline",
            "control-plane",
            "timeline-preparation",
            false,
            "none",
            reasonCode);
    }

    return std::nullopt;
}

MediaPlaybackContract MediaPlaybackContractFactory::failed(
    MediaPlaybackResourceMode resourceMode,
    const std::string& reasonCode)
{
    MediaPlaybackContract contract;
    contract.resourceMode = resourceMode;
    contract.failure = classifyFailure(reasonCode);
    return contract;
}

std::string MediaPlaybackContractFactory::json(const MediaPlaybackContract& contract)
{
    std::string result =
        "{\"contractVersion\":" + std::to_string(contract.contractVersion) +
        ",\"resourceMode\":\"" + std::string(resourceModeName(contract.resourceMode)) + "\"" +
        ",\"presentationProfileId\":" + nullableString(contract.presentationProfileId) +
        ",\"playback\":{"
            "\"positionSeconds\":" + optionalIntJson(contract.positionSeconds) +
            ",\"durationSeconds\":" + optionalIntJson(contract.durationSeconds) +
            ",\"presentationBasePositionSeconds\":" +
                optionalIntJson(contract.presentationBasePositionSeconds) +
            ",\"pauseSupported\":" + optionalBoolJson(contract.pauseSupported) +
            ",\"resumeSupported\":" + optionalBoolJson(contract.resumePlaybackSupported) +
            ",\"restart\":{"
                "\"supported\":" + optionalBoolJson(contract.restartSupported) +
                ",\"preparing\":" + optionalBoolJson(contract.restartPreparing) +
            "}}";

    result +=
        ",\"seek\":{"
            "\"supported\":" + optionalBoolJson(contract.seek.supported) +
            ",\"mode\":\"" + std::string(seekModeName(contract.seek.mode)) + "\"" +
            ",\"preparing\":" + optionalBoolJson(contract.seek.preparing);
    if (contract.seek.windowStartSeconds.has_value() &&
        contract.seek.windowEndSeconds.has_value() &&
        *contract.seek.windowEndSeconds > *contract.seek.windowStartSeconds) {
        result +=
            ",\"window\":{\"startSeconds\":" +
                optionalIntJson(contract.seek.windowStartSeconds) +
            ",\"endSeconds\":" +
                optionalIntJson(contract.seek.windowEndSeconds) + "}";
    }
    result += "}";

    result +=
        ",\"tracks\":{"
            "\"audioSelection\":{\"supported\":" +
                optionalBoolJson(contract.tracks.audioSelectionSupported) + "}," +
            "\"subtitleSelection\":{\"supported\":" +
                optionalBoolJson(contract.tracks.subtitleSelectionSupported) + "}," +
            "\"subtitleOff\":{\"supported\":" +
                optionalBoolJson(contract.tracks.subtitleOffSupported) + "}}";

    result +=
        ",\"continuity\":{"
            "\"generation\":" + optionalIntJson(contract.continuityGeneration) +
            ",\"state\":" + optionalStringJson(contract.continuityState) + "}";

    result += ",\"failure\":";
    if (!contract.failure.has_value()) {
        result += "null";
    }
    else {
        result +=
            "{\"category\":\"" + jsonEscape(contract.failure->category) + "\"," +
            "\"origin\":\"" + jsonEscape(contract.failure->origin) + "\"," +
            "\"stage\":\"" + jsonEscape(contract.failure->stage) + "\"," +
            "\"terminal\":" + std::string(contract.failure->terminal ? "true" : "false") + "," +
            "\"recoveryClass\":\"" + jsonEscape(contract.failure->recoveryClass) + "\"," +
            "\"reasonCode\":\"" + jsonEscape(contract.failure->reasonCode) + "\"}";
    }
    result += "}";
    return result;
}

std::string MediaPlaybackContractFactory::legacyPlaybackJson(
    const MediaPlaybackContract& contract)
{
    const bool inSessionSeek =
        contract.seek.mode == MediaPlaybackSeekMode::InSessionReposition;
    const std::optional<bool> seekSupported = inSessionSeek
        ? contract.seek.supported
        : std::optional<bool>{false};
    const std::optional<bool> seekPreparing = inSessionSeek
        ? contract.seek.preparing
        : std::optional<bool>{false};

    std::string result =
        "{\"positionSeconds\":" + optionalIntJson(contract.positionSeconds) +
        ",\"durationSeconds\":" + optionalIntJson(contract.durationSeconds) +
        ",\"seek\":{\"supported\":" + optionalBoolJson(seekSupported) +
        ",\"preparing\":" + optionalBoolJson(seekPreparing);
    if (inSessionSeek && optionalTrue(seekSupported) &&
        contract.seek.windowStartSeconds.has_value() &&
        contract.seek.windowEndSeconds.has_value()) {
        result +=
            ",\"window\":{\"startSeconds\":" +
                optionalIntJson(contract.seek.windowStartSeconds) +
            ",\"endSeconds\":" +
                optionalIntJson(contract.seek.windowEndSeconds) + "}";
    }
    result +=
        "},\"resume\":{\"supported\":" + optionalBoolJson(contract.restartSupported) +
        ",\"preparing\":" + optionalBoolJson(contract.restartPreparing) + "}}";
    return result;
}
