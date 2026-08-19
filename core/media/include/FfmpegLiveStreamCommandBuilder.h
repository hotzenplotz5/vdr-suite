#pragma once

#include <string>
#include <vector>

struct FfmpegLiveStreamCommandPlan
{
    bool valid = false;
    std::string reasonCode;
    std::vector<std::string> argv;
};

class FfmpegLiveStreamCommandBuilder
{
public:
    FfmpegLiveStreamCommandPlan build(
        const std::string& unixSocketPath,
        const std::string& outputPath) const;
};
