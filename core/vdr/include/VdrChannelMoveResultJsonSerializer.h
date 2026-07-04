
#pragma once

#include "VdrChannelMoveResult.h"

#include <string>

class VdrChannelMoveResultJsonSerializer
{
public:
    std::string serialize(
        const VdrChannelMoveResult& result) const;
};
