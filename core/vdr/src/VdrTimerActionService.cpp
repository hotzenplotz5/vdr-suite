#include "VdrTimerActionService.h"

VdrTimerActionResult VdrTimerActionService::create(
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor) const
{
    return execute(VdrTimerActionType::Create, request, executor);
}

VdrTimerActionResult VdrTimerActionService::update(
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor) const
{
    return execute(VdrTimerActionType::Update, request, executor);
}

VdrTimerActionResult VdrTimerActionService::remove(
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor) const
{
    return execute(VdrTimerActionType::Delete, request, executor);
}

VdrTimerActionResult VdrTimerActionService::toggle(
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor) const
{
    return execute(VdrTimerActionType::Toggle, request, executor);
}

VdrTimerActionResult VdrTimerActionService::execute(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor) const
{
    return executor.execute(type, request);
}
