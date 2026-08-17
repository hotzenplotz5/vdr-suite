#pragma once

#include "VdrRecording.h"

#include <functional>
#include <string>
#include <vector>

struct LocalVdrRecordingSource
{
    std::string backendId;
    std::string recordingId;
    std::string recordingDirectory;
    std::vector<std::string> segmentPaths;
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
