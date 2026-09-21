
#pragma once

#include <string>

struct VdrChannelMoveRequest
{
    std::string backendId = "default";
    std::string channelId;
    int sourceNumber = 0;
    int targetNumber = 0;
    bool dryRun = false;
};
