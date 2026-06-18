#pragma once

#include "IVdrTimerActionExecutor.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "VdrTimerActionResult.h"
#include "VdrTimerOperationRequest.h"

class VdrTimerActionExecutionService
{
public:
    VdrTimerActionResult execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor) const;

    VdrTimerActionResult execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request,
        const VdrTimerActionExecutorAdapterRegistry& registry) const;
};
