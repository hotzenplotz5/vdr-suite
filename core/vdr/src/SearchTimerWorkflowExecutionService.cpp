#include "SearchTimerWorkflowExecutionService.h"

SearchTimerWorkflowExecutionResult SearchTimerWorkflowExecutionService::executePlan(
    const SearchTimerWorkflowExecutionPlan& plan,
    bool explicitOperatorConfirmation) const
{
    if (!plan.valid() || !plan.hasExecutionWork())
    {
        return SearchTimerWorkflowExecutionResult::blockedResult(
            plan,
            "workflow plan is not executable",
            {"workflow plan is not executable"});
    }

    if (plan.requiresExplicitOperatorConfirmation() &&
        !explicitOperatorConfirmation)
    {
        return SearchTimerWorkflowExecutionResult::blockedResult(
            plan,
            "explicit operator confirmation is required",
            {"explicit operator confirmation is required"});
    }

    std::vector<std::string> warnings;
    warnings.push_back(
        "backend execution is not implemented in this skeleton");

    if (plan.requiresBackendReadback())
    {
        warnings.push_back(
            "backend readback will be required after real execution");
    }

    if (plan.writeOperation())
    {
        return SearchTimerWorkflowExecutionResult::acceptedSkeleton(
            plan,
            explicitOperatorConfirmation,
            "write workflow plan accepted by execution skeleton",
            warnings);
    }

    return SearchTimerWorkflowExecutionResult::acceptedSkeleton(
        plan,
        explicitOperatorConfirmation,
        "read-only workflow plan accepted by execution skeleton",
        warnings);
}
