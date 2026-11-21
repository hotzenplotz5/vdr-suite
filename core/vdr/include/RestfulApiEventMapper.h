#pragma once

#include "VdrEvent.h"

#include <string>
#include <vector>

class RestfulApiEventMapper {
public:
    static std::vector<VdrEvent> parseEvents(const std::string& json);
};
