#pragma once

#include "MediaCapabilities.h"

#include <string>

struct RecordingSubtitleSidecarResult
{
    bool available = false;
    std::string reasonCode;
    std::string path;
    MediaSubtitleStreamDescriptor track;
};

class RecordingSubtitleSidecar
{
public:
    static RecordingSubtitleSidecarResult discover(
        const std::string& recordingDirectory);

    static bool appendTo(
        MediaSourceDescriptor& source,
        const std::string& recordingDirectory);
};
