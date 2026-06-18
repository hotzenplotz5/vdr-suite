#pragma once

#include "IVdrTimerActionExecutor.h"
#include "VdrTimerActionResult.h"
#include "VdrTimerOperationRequest.h"

class VdrTimerActionService
{
public:
    VdrTimerActionResult create(
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor) const;

    VdrTimerActionResult update(
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor) const;

    VdrTimerActionResult remove(
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor) const;

    VdrTimerActionResult toggle(
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor) const;

private:
    VdrTimerActionResult execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor) const;
};
