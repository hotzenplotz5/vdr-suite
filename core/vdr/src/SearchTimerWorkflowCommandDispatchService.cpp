#include "SearchTimerWorkflowCommandDispatchService.h"

#include "SearchTimerWorkflowCommandRequestMapper.h"

#include <string>
#include <vector>

namespace
{

std::string commandNameForStep(
    SearchTimerWorkflowExecutionStep step)
{
    if (step == SearchTimerWorkflowExecutionStep::Create)
    {
        return "create";
    }

    if (step == SearchTimerWorkflowExecutionStep::Update)
    {
        return "update";
    }

    if (step == SearchTimerWorkflowExecutionStep::Delete)
    {
        return "delete";
    }

    return "none";
}

} // namespace

SearchTimerWorkflowExecutionResult
SearchTimerWorkflowCommandDispatchService::dispatchPlan(
    const SearchTimerWorkflowExecutionPlan& plan,
    bool explicitOperatorConfirmation) const
{
    if (!plan.valid() || !plan.hasExecutionWork())
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::blockedResult(
                plan,
                "workflow plan is not executable",
                {"workflow plan is not executable"});
        result.dispatchStage = "validation-blocked";
        return result;
    }

    if (plan.requiresExplicitOperatorConfirmation() &&
        !explicitOperatorConfirmation)
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::blockedResult(
                plan,
                "explicit operator confirmation is required",
                {"explicit operator confirmation is required"});
        result.dispatchStage = "confirmation-required";
        return result;
    }

    if (!plan.writeOperation())
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::acceptedSkeleton(
                plan,
                explicitOperatorConfirmation,
                "read-only workflow plan has no command dispatch work",
                {"no command request is required for read-only workflow step"});
        result.dispatchStage = "read-only-no-dispatch";
        return result;
    }

    SearchTimerWorkflowCommandRequestMapper mapper;
    std::string commandName =
        commandNameForStep(plan.primaryStep());

    if (mapper.canBuildCreateRequest(plan))
    {
        const SearchTimerCreateRequest request =
            mapper.buildCreateRequest(plan);
        (void)request;
    }
    else if (mapper.canBuildUpdateRequest(plan))
    {
        const SearchTimerUpdateRequest request =
            mapper.buildUpdateRequest(plan);
        (void)request;
    }
    else if (mapper.canBuildDeleteRequest(plan))
    {
        const SearchTimerDeleteRequest request =
            mapper.buildDeleteRequest(plan);
        (void)request;
    }
    else
    {
        SearchTimerWorkflowExecutionResult result =
            SearchTimerWorkflowExecutionResult::blockedResult(
                plan,
                "workflow plan cannot be mapped to a command request",
                {"workflow plan cannot be mapped to a command request"});
        result.dispatchStage = "command-request-mapping-failed";
        return result;
    }

    std::vector<std::string> warnings;
    warnings.push_back(
        "command request mapped by dispatch skeleton");
    warnings.push_back(
        "backend command dispatch is not enabled in this skeleton");

    if (plan.requiresBackendReadback())
    {
        warnings.push_back(
            "backend readback will be required after real execution");
    }

    SearchTimerWorkflowExecutionResult result =
        SearchTimerWorkflowExecutionResult::acceptedSkeleton(
            plan,
            explicitOperatorConfirmation,
            commandName + " command request accepted by dispatch skeleton",
            warnings);
    result.commandRequestMapped = true;
    result.realExecutionEnabled = false;
    result.dispatchStage = "command-request-mapped";
    return result;
}
