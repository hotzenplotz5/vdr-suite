#include "SearchTimerWorkflowBackendWritePermissionGate.h"

namespace
{

bool containsBackendId(
    const std::vector<std::string>& backendIds,
    const std::string& backendId)
{
    for (const std::string& value : backendIds)
    {
        if (value == backendId)
        {
            return true;
        }
    }

    return false;
}

} // namespace

SearchTimerWorkflowBackendWritePermissionGateDecision
SearchTimerWorkflowBackendWritePermissionGate::evaluate(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerWorkflowCommandDispatchOptions& options) const
{
    SearchTimerWorkflowBackendWritePermissionGateDecision decision;
    decision.backendId = plan.backendId();
    decision.permittedBackendIds = options.backendWritePermittedBackendIds();
    decision.configured = options.backendWritePermissionGateEnabled();

    if (!plan.writeOperation())
    {
        decision.permitted = true;
        decision.dispatchStage = "backend-write-permission-not-required";
        decision.message = "backend write permission is not required for read-only workflow";
        decision.errors.clear();
        return decision;
    }

    if (!decision.configured)
    {
        decision.permitted = false;
        decision.dispatchStage = "backend-write-permission-required";
        decision.message = "backend write permission gate is required";
        decision.errors = {"backend write permission gate is required"};
        return decision;
    }

    if (decision.backendId.empty())
    {
        decision.permitted = false;
        decision.dispatchStage = "backend-write-permission-backend-id-required";
        decision.message = "backend write permission requires a backend id";
        decision.errors = {"backend write permission requires a backend id"};
        return decision;
    }

    if (!containsBackendId(decision.permittedBackendIds, decision.backendId))
    {
        decision.permitted = false;
        decision.dispatchStage = "backend-write-permission-denied";
        decision.message = "backend is not permitted for SearchTimer write execution";
        decision.errors = {"backend is not permitted for SearchTimer write execution"};
        return decision;
    }

    if (options.hasBackendAccessPolicy())
    {
        const BackendAccessDecision accessDecision =
            options.backendAccessPolicy()->canWriteToBackend(
                *options.backendRegistryService(),
                decision.backendId);

        if (!accessDecision.allowed)
        {
            decision.permitted = false;
            decision.configured = true;
            decision.dispatchStage = "backend-write-access-denied";
            decision.message = accessDecision.reason;
            decision.errors = accessDecision.errors;

            if (decision.errors.empty())
            {
                decision.errors.push_back(accessDecision.reason);
            }

            return decision;
        }
    }

    decision.permitted = true;
    decision.dispatchStage = "backend-write-permission-permitted";
    decision.message = "backend is permitted for SearchTimer write execution";
    decision.errors.clear();
    return decision;
}
