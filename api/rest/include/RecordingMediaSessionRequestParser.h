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

struct RecordingMediaSessionStopRequest
{
    bool valid = false;
    std::string reasonCode;
    std::string backendId;
    std::string sessionId;
};

struct RecordingMediaSessionSeekRequest
{
    bool valid = false;
    std::string reasonCode;
    std::string backendId;
    std::string sessionId;
    int positionSeconds = 0;
};

struct RecordingMediaSessionPlaybackStatusRequest
{
    bool valid = false;
    std::string reasonCode;
    std::string backendId;
    std::string sessionId;
};

struct RecordingMediaSessionAudioTrackSelectionRequest
{
    bool valid = false;
    std::string reasonCode;
    std::string backendId;
    std::string sessionId;
    std::string audioTrackId;
    int positionSeconds = 0;
};

class RecordingMediaSessionRequestParser
{
public:
    RecordingMediaSessionRequest parse(
        const std::string& body) const;

    RecordingMediaSessionStopRequest parseStop(
        const std::string& body) const;

    RecordingMediaSessionSeekRequest parseSeek(
        const std::string& body) const;

    RecordingMediaSessionPlaybackStatusRequest parsePlaybackStatus(
        const std::string& body) const;

    RecordingMediaSessionAudioTrackSelectionRequest parseAudioTrackSelection(
        const std::string& body) const;
};
