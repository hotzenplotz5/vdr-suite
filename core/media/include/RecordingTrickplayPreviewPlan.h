#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Internal, authorization-neutral command planner. Never accepts a client
// path, codec claim, URL or backend/device identifier. Its caller MUST first
// resolve the recording under an authenticated Device/Browser principal and
// prepare an index-derived, privately owned MediaSessionWorkspace.
struct RecordingTrickplayPreviewPlan
{
    bool valid = false;
    std::string reasonCode;
    int requestedSeconds = 0;
    int sampleSeconds = 0;
    int intervalSeconds = 10;
    std::vector<std::string> argv;
};

class RecordingTrickplayPreviewPlanner
{
public:
    // Source and durations must come from the same trusted recording index.
    // A missing/partial index cannot authorize arbitrary time offsets.
    RecordingTrickplayPreviewPlan build(
        int requestedSeconds,
        int indexedDurationSeconds,
        std::size_t verifiedSourceSegmentCount,
        const std::vector<double>& verifiedSegmentDurationsSeconds) const;
};
