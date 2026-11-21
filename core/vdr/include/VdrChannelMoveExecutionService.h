
#pragma once

#include "BackendAccessPolicy.h"
#include "VdrChannelMoveExecutorAdapterRegistry.h"
#include "VdrChannelMoveRequest.h"
#include "VdrChannelMoveResult.h"

class VdrChannelMoveExecutionService
{
public:
    VdrChannelMoveResult execute(
        const VdrChannelMoveRequest& request,
        const VdrChannelMoveExecutorAdapterRegistry& registry,
        const BackendAccessDecision& accessDecision) const;
};
