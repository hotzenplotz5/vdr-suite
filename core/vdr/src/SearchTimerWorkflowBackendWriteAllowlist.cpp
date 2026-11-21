#include "SearchTimerWorkflowBackendWriteAllowlist.h"

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

SearchTimerWorkflowBackendWriteAllowlistDecision
SearchTimerWorkflowBackendWriteAllowlist::evaluate(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerWorkflowCommandDispatchOptions& options) const
{
    SearchTimerWorkflowBackendWriteAllowlistDecision decision;
    decision.backendId = plan.backendId();
    decision.allowedBackendIds = options.backendWriteAllowedBackendIds();
    decision.configured = options.backendWriteAllowlistEnabled();

    if (!plan.writeOperation())
    {
        decision.allowed = true;
        decision.dispatchStage = "backend-write-allowlist-not-required";
        decision.message = "backend write allowlist is not required for read-only workflow";
        decision.errors.clear();
        return decision;
    }

    if (!decision.configured)
    {
        decision.allowed = false;
        decision.dispatchStage = "backend-write-allowlist-required";
        decision.message = "backend write allowlist is required";
        decision.errors = {"backend write allowlist is required"};
        return decision;
    }

    if (decision.backendId.empty())
    {
        decision.allowed = false;
        decision.dispatchStage = "backend-write-backend-id-required";
        decision.message = "backend write allowlist requires a backend id";
        decision.errors = {"backend write allowlist requires a backend id"};
        return decision;
    }

    if (!containsBackendId(decision.allowedBackendIds, decision.backendId))
    {
        decision.allowed = false;
        decision.dispatchStage = "backend-write-backend-not-allowed";
        decision.message = "backend is not allowed for SearchTimer write execution";
        decision.errors = {"backend is not allowed for SearchTimer write execution"};
        return decision;
    }

    decision.allowed = true;
    decision.dispatchStage = "backend-write-allowlist-allowed";
    decision.message = "backend is allowed for SearchTimer write execution";
    decision.errors.clear();
    return decision;
}
