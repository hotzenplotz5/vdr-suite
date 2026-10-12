#pragma once

#include "LocalVdrRecordingSourceResolver.h"
#include "VdrRecordingDuration.h"

#include <string>

// Read-only Device projection of the same index/segment eligibility used by
// RecordingMediaSessionController::create. A known duration alone is NOT a
// valid HLS resume timeline. Never expose native paths or media credentials.
struct DeviceRecordingResumeProjection
{
    int durationSeconds = 0;
    bool supported = false;
    bool preparing = false;
    // Fixed, path-free diagnostic vocabulary; never return native source
    // resolver messages to the paired Device.
    const char* reasonCode = "recording_resume_not_ready";
};

inline DeviceRecordingResumeProjection projectDeviceRecordingResume(
    const VdrRecording& recording,
    const LocalVdrRecordingSourceResolution& resolved,
    const std::string& presentationProfileId,
    bool indexPreparing)
{
    DeviceRecordingResumeProjection result;
    result.durationSeconds =
        vdrsuite::recording::durationSecondsFromIndex(recording);
    result.preparing = indexPreparing;

    const bool hls = presentationProfileId == "hls-fmp4" ||
        presentationProfileId == "hls-ts";
    if (!hls) {
        result.reasonCode = "recording_resume_profile_not_supported";
        return result;
    }
    if (result.preparing) {
        result.reasonCode = "recording_resume_preparing";
        return result;
    }
    if (!resolved.resolved || resolved.source.segmentPaths.empty()) {
        result.reasonCode = "recording_resume_source_unavailable";
        return result;
    }
    if (resolved.source.growing) {
        result.reasonCode = "recording_resume_growing_not_supported";
        return result;
    }
    if (result.durationSeconds <= 0) {
        result.reasonCode = "recording_resume_index_unavailable";
        return result;
    }

    const std::vector<double> durations =
        vdrsuite::recording::segmentDurationsSecondsFromIndex(
            recording, resolved.source.segmentPaths);
    result.supported = !durations.empty() &&
        durations.size() == resolved.source.segmentPaths.size();
    result.reasonCode = result.supported ? "" :
        "recording_resume_segment_timeline_unavailable";
    return result;
}

inline std::string deviceRecordingResumePlaybackJson(
    const DeviceRecordingResumeProjection& projection)
{
    return "{\"durationSeconds\":" +
        (projection.durationSeconds > 0
            ? std::to_string(projection.durationSeconds)
            : "null") +
        ",\"resume\":{\"supported\":" +
        (projection.supported ? "true" : "false") +
        ",\"preparing\":" +
        (projection.preparing ? "true" : "false") +
        ",\"reasonCode\":" +
        (projection.supported ? "null" :
            "\"" + std::string(projection.reasonCode) + "\"") +
        "}}";
}
