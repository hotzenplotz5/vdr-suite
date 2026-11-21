#pragma once

#include "MediaCapabilities.h"

#include <cstddef>
#include <string>

class RecordingMediaTrackContract
{
public:
    static std::string audioTrackId(std::size_t sourceAudioStreamIndex);
    static std::string subtitleTrackId(std::size_t sourceSubtitleStreamIndex);

    static bool audioStreamIndexForTrackId(
        const std::string& trackId,
        const MediaSourceDescriptor& source,
        int& sourceAudioStreamIndex);

    static bool subtitleStreamIndexForTrackId(
        const std::string& trackId,
        const MediaSourceDescriptor& source,
        int& sourceSubtitleStreamIndex);

    static bool subtitleTrackSelectable(MediaSubtitleFormat format);

    // subtitleOffSelectedState: -1 unknown, 0 not off, 1 off.
    static std::string json(
        const MediaSourceDescriptor& source,
        int selectedAudioStreamIndex,
        bool audioSelectionSupported,
        const std::string& audioSelectionReason,
        bool subtitleSelectionSupported = false,
        const std::string& subtitleSelectionReason =
            "profile_does_not_deliver_selectable_subtitles",
        bool subtitleOffSupported = true,
        int subtitleOffSelectedState = 1,
        int selectedSubtitleStreamIndex = -1);
};
