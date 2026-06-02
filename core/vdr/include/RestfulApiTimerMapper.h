#pragma once

#include "VdrTimer.h"

#include <string>
#include <vector>

class RestfulApiTimerMapper {
public:
    static std::vector<VdrTimer> parseTimers(const std::string& json);
};
