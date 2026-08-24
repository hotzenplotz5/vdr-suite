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

// Compare the immutable Recording extent encoded by two v1 fingerprints while
// deliberately ignoring only the final growing-state bit. The extent includes
// directory, segment names, device/inode identity, sizes and mtimes, so callers
// can distinguish a transient VDR state marker from an actually changing
// Recording source without weakening the conservative growing-source policy.
bool sameRecordingSourceExtentIgnoringGrowthState(
    const std::string& left,
    const std::string& right);
