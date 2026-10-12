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
    if (!hls || result.preparing || result.durationSeconds <= 0 ||
        !resolved.resolved || resolved.source.growing ||
        resolved.source.segmentPaths.empty())
        return result;

    const std::vector<double> durations =
        vdrsuite::recording::segmentDurationsSecondsFromIndex(
            recording, resolved.source.segmentPaths);
    result.supported = !durations.empty() &&
        durations.size() == resolved.source.segmentPaths.size();
    return result;
}

inline std::string deviceRecordingResumePlaybackJson(
    const DeviceRecordingResumeProjection& projection)
{
    return "{\\\"durationSeconds\\\":" +
        (projection.durationSeconds > 0
            ? std::to_string(projection.durationSeconds)
            : "null") +
        ",\\\"resume\\\":{\\\"supported\\\":" +
        (projection.supported ? "true" : "false") +
        ",\\\"preparing\\\":" +
        (projection.preparing ? "true" : "false") +
        "}}";
}
