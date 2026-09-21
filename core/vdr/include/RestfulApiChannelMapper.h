#pragma once

#include "VdrChannel.h"

#include <string>
#include <vector>

class RestfulApiChannelMapper {
public:
    static std::vector<VdrChannel> parseChannels(const std::string& json);
};
