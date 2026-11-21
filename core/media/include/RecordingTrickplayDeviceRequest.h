#pragma once

#include <string>

// Strict, bounded, canonical Device GET URI for a single already-owned
// media session. Parsing does NOT authorize a Device or a recording.
struct RecordingTrickplayDeviceRequest
{
    bool valid = false;
    std::string reasonCode;
    std::string sessionId;
    int positionSeconds = 0;
};

class RecordingTrickplayDeviceRequestParser
{
public:
    RecordingTrickplayDeviceRequest parse(
        const std::string& requestTarget) const;
};
