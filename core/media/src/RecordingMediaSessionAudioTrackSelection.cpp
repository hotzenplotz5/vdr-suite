#include "RecordingMediaSessionRuntime.h"

namespace
{

bool sameVideoPresentation(
    const MediaPresentationProfile& left,
    const MediaPresentationProfile& right)
{
    return left.profileId == right.profileId &&
        left.protocol == right.protocol &&
        left.container == right.container &&
        left.videoAction == right.videoAction &&
        left.sourceVideoStreamIndex == right.sourceVideoStreamIndex &&
        left.targetVideoCodec == right.targetVideoCodec &&
        left.targetVideoWidth == right.targetVideoWidth &&
        left.targetVideoHeight == right.targetVideoHeight &&
        left.deinterlaceVideo == right.deinterlaceVideo &&
        left.videoTranscodeWorkload == right.videoTranscodeWorkload &&
        left.videoEncoderBackend == right.videoEncoderBackend &&
        left.videoEncoderPreset == right.videoEncoderPreset &&
        left.videoHardwareDevice == right.videoHardwareDevice;
}

} // namespace

RecordingMediaSessionAudioTrackSelectionResult
RecordingMediaSessionRuntime::selectAudioTrack(
    const std::string& sessionId,
    const MediaPresentationProfile& profile,
    int positionSeconds)
{
    RecordingMediaSessionAudioTrackSelectionResult result;
    if (sessionId.empty() || positionSeconds < 0 || !profile.available ||
        profile.profileId != "progressive-fmp4" ||
        profile.protocol != MediaDeliveryProtocol::Progressive ||
        profile.container != MediaContainer::Fmp4 ||
        profile.sourceAudioStreamIndex < 0 ||
        profile.audioAction == MediaTrackAction::Omit) {
        result.reasonCode = "invalid_recording_audio_track_selection";
        return result;
    }

    const MediaPresentationProfile resolvedProfile = transcodePolicy_.apply(profile);
    if (!resolvedProfile.available) {
        result.reasonCode = resolvedProfile.reason.empty()
            ? "recording_audio_track_selection_transcode_unavailable"
            : resolvedProfile.reason;
        return result;
    }

    MediaPresentationProfile previousProfile;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = active_.find(sessionId);
        if (found == active_.end()) {
            result.reasonCode = "recording_audio_track_runtime_not_found";
            return result;
        }

        ActiveSession& active = found->second;
        if (!active.continuousStream || !active.workspace ||
            !active.indexedSeekTimeline || active.durationSeconds <= 0 ||
            active.streamProfile.profileId != "progressive-fmp4") {
            result.reasonCode = "recording_audio_track_selection_not_supported";
            return result;
        }
        if (positionSeconds >= active.durationSeconds) {
            result.reasonCode = "recording_audio_track_position_outside_window";
            result.durationSeconds = active.durationSeconds;
            return result;
        }
        if (!sameVideoPresentation(active.streamProfile, resolvedProfile)) {
            result.reasonCode = "recording_audio_track_selection_video_change_not_allowed";
            return result;
        }

        result.sourceAudioStreamIndex = resolvedProfile.sourceAudioStreamIndex;
        result.positionSeconds = positionSeconds;
        result.durationSeconds = active.durationSeconds;
        if (active.streamProfile.sourceAudioStreamIndex ==
                resolvedProfile.sourceAudioStreamIndex &&
            active.streamProfile.audioAction == resolvedProfile.audioAction &&
            active.streamProfile.targetAudioCodec == resolvedProfile.targetAudioCodec &&
            active.streamProfile.targetAudioChannels == resolvedProfile.targetAudioChannels) {
            result.selected = true;
            return result;
        }

        previousProfile = active.streamProfile;
        active.streamProfile = resolvedProfile;
    }

    const RecordingMediaSessionSeekResult restart =
        seekStream(sessionId, positionSeconds);
    if (restart.repositioned) {
        result.selected = true;
        result.restarted = true;
        result.positionSeconds = restart.positionSeconds;
        result.durationSeconds = restart.durationSeconds;
        return result;
    }

    // seekStream owns the actual worker replacement. If it failed before the
    // old stream became terminal, restore the previously selected profile so
    // a later retry cannot accidentally advertise or restart the wrong track.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = active_.find(sessionId);
        if (found != active_.end() &&
            found->second.streamProfile.sourceAudioStreamIndex ==
                resolvedProfile.sourceAudioStreamIndex) {
            found->second.streamProfile = previousProfile;
        }
    }
    result.reasonCode = restart.reasonCode.empty()
        ? "recording_audio_track_restart_failed"
        : restart.reasonCode;
    result.durationSeconds = restart.durationSeconds;
    return result;
}
