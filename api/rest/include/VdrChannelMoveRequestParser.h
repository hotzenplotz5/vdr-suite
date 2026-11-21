
#pragma once

#include "VdrChannelMoveRequest.h"

#include <string>

class VdrChannelMoveRequestParser
{
public:
    VdrChannelMoveRequest parse(
        const std::string& body) const;
};
