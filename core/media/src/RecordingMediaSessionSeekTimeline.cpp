#include "RecordingMediaSessionRuntime.h"

#include "MediaSessionWorkspace.h"

RecordingMediaSessionSeekCapabilityResult
RecordingMediaSessionRuntime::enableIndexedSeek(
    const std::string& sessionId,
    int durationSeconds,
    const std::vector<std::string>& sourceSegments,
    const std::vector<double>& segmentDurationsSeconds)
{
    RecordingMediaSessionSeekCapabilityResult result;
    if (sessionId.empty() || durationSeconds <= 0 || sourceSegments.empty() ||
        segmentDurationsSeconds.size() != sourceSegments.size()) {
        result.reasonCode = "invalid_recording_seek_capability_request";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = active_.find(sessionId);
    if (found == active_.end()) {
        result.reasonCode = "recording_seek_runtime_not_found";
        return result;
    }

    ActiveSession& active = found->second;
    if (!active.continuousStream || !active.workspace || active.direct ||
        active.streamProfile.profileId != "progressive-fmp4") {
        result.reasonCode = "recording_seek_not_supported";
        return result;
    }

    if (active.indexedSeekTimeline) {
        result.enabled = active.durationSeconds > 0;
        result.durationSeconds = active.durationSeconds;
        if (!result.enabled)
            result.reasonCode = "recording_seek_not_supported";
        return result;
    }

    const MediaSessionWorkspaceResult workspaceResult =
        active.workspace->activateSeekTimeline(
            sourceSegments,
            segmentDurationsSeconds);
    if (!workspaceResult.ready) {
        result.reasonCode = workspaceResult.reasonCode.empty()
            ? "recording_seek_timeline_activation_failed"
            : workspaceResult.reasonCode;
        return result;
    }

    active.durationSeconds = durationSeconds;
    active.indexedSeekTimeline = true;
    result.enabled = true;
    result.durationSeconds = durationSeconds;
    return result;
}
