#pragma once

#include "MediaCapabilities.h"

#include <string>

struct LiveMediaSessionRequest
{
    bool valid = false;
    std::string reasonCode;
    std::string backendId;
    std::string channelId;
    std::string replacesSessionId;
    ClientMediaCapabilities capabilities;
};

class LiveMediaSessionRequestParser
{
public:
    LiveMediaSessionRequest parse(const std::string& body) const;
    static bool requestsLiveChannel(const std::string& body);
};
