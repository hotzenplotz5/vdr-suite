#include "VdrTimerActionExecutionService.h"

VdrTimerActionResult VdrTimerActionExecutionService::execute(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor) const
{
    return executor.execute(
        type,
        request);
}

VdrTimerActionResult VdrTimerActionExecutionService::execute(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request,
    const VdrTimerActionExecutorAdapterRegistry& registry) const
{
    const VdrTimerActionExecutorAdapterLookupResult resolvedAdapter =
        registry.findAdapter(request.backendId);

    if (!resolvedAdapter.found || !resolvedAdapter.adapter)
    {
        return VdrTimerActionResult::failed(
            type,
            request.timerId,
            request.backendId,
            resolvedAdapter.message,
            {resolvedAdapter.message});
    }

    return resolvedAdapter.adapter->executor().execute(
        type,
        request);
}
