#pragma once

#include <string>

struct RecordingMediaSessionStartPosition
{
    bool valid = false;
    bool present = false;
    int seconds = 0;
    std::string reasonCode;
};

class RecordingMediaSessionStartPositionParser
{
public:
    RecordingMediaSessionStartPosition parse(
        const std::string& body) const;
};
