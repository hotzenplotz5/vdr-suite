#pragma once

#include <string>

struct RecordingMediaSessionAudioTrackPreference
{
    bool valid = false;
    std::string reasonCode;
    std::string audioTrackId;
};

class RecordingMediaSessionAudioTrackPreferenceParser
{
public:
    RecordingMediaSessionAudioTrackPreference parse(
        const std::string& body) const;
};
