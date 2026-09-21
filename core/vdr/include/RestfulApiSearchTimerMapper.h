#pragma once

#include "SearchTimer.h"

#include <string>
#include <vector>

class RestfulApiSearchTimerMapper {
public:
    static std::vector<SearchTimer> parseSearchTimers(
        const std::string& backendId,
        const std::string& json);
};