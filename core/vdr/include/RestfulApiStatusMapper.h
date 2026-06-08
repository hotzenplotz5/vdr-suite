#pragma once

#include "VdrConfig.h"
#include "VdrStatus.h"

#include <string>

class RestfulApiStatusMapper {
public:
    static VdrStatus parseStatus(const std::string& json, const VdrConfig& config, int statusCode);
};
