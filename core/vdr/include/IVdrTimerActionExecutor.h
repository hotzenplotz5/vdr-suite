#pragma once

#include "VdrTimerActionResult.h"
#include "VdrTimerOperationRequest.h"

class IVdrTimerActionExecutor
{
public:
    virtual ~IVdrTimerActionExecutor() = default;

    virtual VdrTimerActionResult execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request) = 0;
};
