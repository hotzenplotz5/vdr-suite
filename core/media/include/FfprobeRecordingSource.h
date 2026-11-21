#pragma once

#include "MediaCapabilities.h"

#include <string>
#include <vector>

struct FfprobeRecordingPlan
{
    std::vector<std::string> argv;
};

struct FfprobeRecordingResult
{
    bool valid = false;
    std::string reasonCode;
    MediaSourceDescriptor source;
};

class FfprobeRecordingSource
{
public:
    FfprobeRecordingPlan commandPlan() const;

    FfprobeRecordingResult parse(
        const std::string& output) const;
};
