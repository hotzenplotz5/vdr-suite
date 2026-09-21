
#pragma once

#include "VdrChannelMoveRequest.h"
#include "VdrChannelMoveResult.h"

class IVdrChannelMoveExecutor
{
public:
    virtual ~IVdrChannelMoveExecutor() = default;

    virtual VdrChannelMoveResult moveChannel(
        const VdrChannelMoveRequest& request) = 0;
};
