#pragma once

#include "VdrRecording.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct LocalVdrRecordingSource
{
    std::string backendId;
    std::string recordingId;
    std::string recordingDirectory;
    std::vector<std::string> segmentPaths;
    bool growing = false;
    bool progressiveDirectSafe = false;
    std::uint64_t readableBytes = 0;
    std::string sourceFingerprint;
};

struct LocalVdrRecordingSourceResolution
{
    bool resolved = false;
    std::string reasonCode;
    LocalVdrRecordingSource source;
};

class LocalVdrRecordingSourceResolver
{
public:
    using RecordingCatalog =
        std::function<std::vector<VdrRecording>(const std::string& backendId)>;

    explicit LocalVdrRecordingSourceResolver(
        RecordingCatalog recordingCatalog);

    LocalVdrRecordingSourceResolution resolve(
        const std::string& backendId,
        const std::string& recordingId) const;

    static std::vector<std::string> discoverSegments(
        const std::string& recordingDirectory);

private:
    RecordingCatalog recordingCatalog_;
};
