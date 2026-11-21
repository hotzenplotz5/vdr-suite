#include "RecordingMediaSessionRuntime.h"

RecordingMediaSessionTrackState RecordingMediaSessionRuntime::trackState(
    const std::string& sessionId) const
{
    RecordingMediaSessionTrackState result;
    if (sessionId.empty()) {
        result.reasonCode = "invalid_media_session_id";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = active_.find(sessionId);
    if (found == active_.end()) {
        result.reasonCode = "recording_track_runtime_not_found";
        return result;
    }

    const ActiveSession& active = found->second;
    result.available = true;
    result.profileId = active.streamProfile.profileId;
    result.sourceAudioStreamIndex = active.streamProfile.sourceAudioStreamIndex;
    result.durationSeconds = active.durationSeconds;
    result.audioSelectionSupported =
        active.continuousStream &&
        active.streamProfile.profileId == "progressive-fmp4" &&
        active.indexedSeekTimeline &&
        active.durationSeconds > 0;
    result.reasonCode = result.audioSelectionSupported
        ? "recording_audio_track_selection_supported"
        : "recording_audio_track_selection_not_supported";
    return result;
}
