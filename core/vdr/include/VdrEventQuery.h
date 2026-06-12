#pragma once

#include <string>

struct VdrEventQuery {
    std::string channelId;
    std::string eventId;

    int from = -1;
    int timespan = 0;

    int start = -1;
    int limit = 0;

    int channelEventLimit = 0;

    bool onlyCount = false;
};
