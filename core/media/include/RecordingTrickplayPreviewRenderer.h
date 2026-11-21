#pragma once

#include "MediaProcessRunner.h"
#include "RecordingTrickplayPreviewPlan.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class MediaSessionWorkspace;

struct RecordingTrickplayPreviewResult
{
    bool ready = false;
    std::string reasonCode;
    int sampleSeconds = 0;
    std::string jpeg;
};

// Internal renderer for an already-authorized and prepared, private VDR
// recording workspace. Does NOT implement a public route or grant validation.
class RecordingTrickplayPreviewRenderer
{
public:
    using Capture = std::function<MediaProcessCaptureResult(
        const std::vector<std::string>& argv,
        const std::string& workingDirectory,
        std::chrono::milliseconds timeout,
        std::size_t maximumOutputBytes,
        MediaProcessLimits limits)>;

    explicit RecordingTrickplayPreviewRenderer(Capture capture = {});
    RecordingTrickplayPreviewResult render(
        const RecordingTrickplayPreviewPlan& plan,
        const MediaSessionWorkspace& workspace) const;

private:
    Capture capture_;
};
