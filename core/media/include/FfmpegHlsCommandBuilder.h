#pragma once

#include "MediaCapabilities.h"

#include <string>
#include <vector>

struct FfmpegHlsCommandPlan
{
    bool valid = false;
    std::string reasonCode;
    std::vector<std::string> argv;
};

class FfmpegHlsCommandBuilder
{
public:
    FfmpegHlsCommandPlan build(
        const MediaPresentationProfile& profile) const;
};
