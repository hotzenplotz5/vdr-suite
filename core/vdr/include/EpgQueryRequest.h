#pragma once

#include "EpgQueryScope.h"

#include <string>

struct EpgQueryRequest {
    EpgQueryScope scope = EpgQueryScope::TimeWindow;

    std::string channelId;

    int from = -1;
    int timespan = 0;
    int limit = 0;
};
