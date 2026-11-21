#pragma once

#include "MediaPlannedOutput.h"

#include <string>
#include <vector>

// Evidence from the *emitted fMP4 initialization header*, not the VDR
// source and not the client's actual decoder. Does not prove rendered frames,
// audible sound, HDR output or segment integrity.
struct MediaEmittedFmp4Result
{
    bool inspected = false;
    bool matchesPlannedOutput = false;
    MediaCodec videoCodec = MediaCodec::None;
    MediaCodec audioCodec = MediaCodec::None;
    int videoWidth = 0;
    int videoHeight = 0;
    int audioChannels = 0;
    std::string reasonCode;
};

class MediaEmittedFmp4Probe
{
public:
    // Fixed argv: ffprobe reads ONLY a private, snapshotted init.mp4 file.
    // Never accepts client paths, public media URLs or ffmpeg options.
    std::vector<std::string> commandPlan() const;

    // Strict bounded parser, useful for fixtures and isolated tests.
    MediaEmittedFmp4Result parse(
        const std::string& output,
        const MediaPlannedOutput& trustedPlan) const;

    // Called only by a future authorized MediaSession owner on its own
    // existing 0700 workspace. Not connected to any public API or hot path.
    MediaEmittedFmp4Result inspectOwnedWorkspace(
        const std::string& trustedWorkspace,
        const MediaPlannedOutput& trustedPlan) const;
};
