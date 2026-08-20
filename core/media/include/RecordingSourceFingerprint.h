#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct RecordingSourceFingerprint
{
    bool valid = false;
    std::string reasonCode;
    bool growing = false;
    std::uint64_t readableBytes = 0;
    std::string value;
};

RecordingSourceFingerprint inspectRecordingSource(
    const std::string& recordingDirectory,
    const std::vector<std::string>& segmentPaths);
