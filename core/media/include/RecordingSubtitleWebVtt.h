#pragma once

#include "MediaCapabilities.h"

#include <string>
#include <vector>

struct RecordingSubtitleWebVttPlan
{
    bool valid = false;
    std::string reasonCode;
    std::vector<std::string> argv;
};

class RecordingSubtitleWebVtt
{
public:
    static bool supports(MediaSubtitleFormat format);

    static RecordingSubtitleWebVttPlan build(
        int sourceSubtitleStreamIndex,
        MediaSubtitleFormat format,
        int streamBasePositionSeconds = 0,
        const std::string& externalSourcePath = {});
};
