#pragma once

#include "FfprobeRecordingSource.h"

#include <string>
#include <vector>

struct FfprobeLivePlan
{
    bool valid = false;
    std::string reasonCode;
    std::vector<std::string> argv;
};

class FfprobeLiveSource
{
public:
    FfprobeLivePlan commandPlan(const std::string& unixSocketPath) const;
    FfprobeRecordingResult parse(const std::string& output) const;
};
