#include "SearchTimerWorkflowExecutorResultMapper.h"

namespace
{

SearchTimerWorkflowExecutionResult mappedSuccessBase(
    const SearchTimerWorkflowExecutionPlan& plan,
    const std::string& message,
    const std::vector<std::string>& warnings)
{
    SearchTimerWorkflowExecutionResult result =
        SearchTimerWorkflowExecutionResult::acceptedSkeleton(
            plan,
            true,
            message,
            warnings);

    result.success = true;
    result.executed = true;
    result.blocked = false;
    result.dryRunOnly = false;
    result.commandRequestMapped = true;
    result.realExecutionEnabled = true;
    result.realExecutionPolicyAllowed = true;
    result.executorOptInProvided = true;
    result.executorInjected = true;
    result.executorInvocationGuardPassed = true;
    result.executorInvocationAttempted = true;
    result.executorInvocationKillSwitchOpen = true;
    result.executorInvocationKillSwitchPassed = true;
    result.executorResultMapped = true;
    result.executorResultSuccessful = true;
    result.dispatchStage = "executor-result-mapped";
    result.executorInvocationAuditTrail = {
        "executorInvocationAttempted=true",
        "executorResultMapped=true",
        "executorResultSuccessful=true",
        "finalDispatchStage=executor-result-mapped"};

    return result;
}

SearchTimerWorkflowExecutionResult mappedFailureBase(
    const SearchTimerWorkflowExecutionPlan& plan,
    const std::string& message,
    const std::vector<std::string>& errors)
{
    SearchTimerWorkflowExecutionResult result =
        SearchTimerWorkflowExecutionResult::blockedResult(
            plan,
            message,
            errors);

    result.success = false;
    result.executed = false;
    result.blocked = true;
    result.dryRunOnly = false;
    result.commandRequestMapped = true;
    result.realExecutionEnabled = true;
    result.realExecutionPolicyAllowed = true;
    result.executorOptInProvided = true;
    result.executorInjected = true;
    result.executorInvocationGuardPassed = true;
    result.executorInvocationAttempted = true;
    result.executorInvocationKillSwitchOpen = true;
    result.executorInvocationKillSwitchPassed = true;
    result.executorResultMapped = true;
    result.executorResultSuccessful = false;
    result.dispatchStage = "executor-result-failed";
    result.executorInvocationAuditTrail = {
        "executorInvocationAttempted=true",
        "executorResultMapped=true",
        "executorResultSuccessful=false",
        "finalDispatchStage=executor-result-failed"};

    return result;
}

} // namespace

SearchTimerWorkflowExecutionResult
SearchTimerWorkflowExecutorResultMapper::mapCreateResult(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerCreateResult& executorResult) const
{
    if (!executorResult.success)
    {
        return mappedFailureBase(
            plan,
            executorResult.message,
            executorResult.errors);
    }

    SearchTimerWorkflowExecutionResult result =
        mappedSuccessBase(
            plan,
            executorResult.message,
            executorResult.warnings);

    result.backendId =
        executorResult.searchTimer.backendId();
    result.backendNativeId =
        executorResult.searchTimer.backendNativeId();

    return result;
}

SearchTimerWorkflowExecutionResult
SearchTimerWorkflowExecutorResultMapper::mapUpdateResult(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerUpdateResult& executorResult) const
{
    if (!executorResult.success)
    {
        return mappedFailureBase(
            plan,
            executorResult.message,
            executorResult.errors);
    }

    SearchTimerWorkflowExecutionResult result =
        mappedSuccessBase(
            plan,
            executorResult.message,
            executorResult.warnings);

    result.backendId =
        executorResult.searchTimer.backendId();
    result.backendNativeId =
        executorResult.searchTimer.backendNativeId();

    return result;
}

SearchTimerWorkflowExecutionResult
SearchTimerWorkflowExecutorResultMapper::mapDeleteResult(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerDeleteResult& executorResult) const
{
    if (!executorResult.success)
    {
        return mappedFailureBase(
            plan,
            executorResult.message,
            executorResult.errors);
    }

    SearchTimerWorkflowExecutionResult result =
        mappedSuccessBase(
            plan,
            executorResult.message,
            executorResult.warnings);

    result.backendId =
        executorResult.backendId;
    result.backendNativeId =
        executorResult.backendNativeId;

    return result;
}
