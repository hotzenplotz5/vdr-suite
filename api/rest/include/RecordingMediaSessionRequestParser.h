#pragma once

#include "MediaCapabilities.h"

#include <string>

struct RecordingMediaSessionRequest
{
    bool valid = false;
    std::string reasonCode;
    std::string backendId;
    std::string recordingId;
    ClientMediaCapabilities capabilities;
};

class RecordingMediaSessionRequestParser
{
public:
    RecordingMediaSessionRequest parse(
        const std::string& body) const;
};
