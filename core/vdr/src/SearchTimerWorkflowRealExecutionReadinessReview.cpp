#include "SearchTimerWorkflowRealExecutionReadinessReview.h"

namespace
{

void addCondition(
    std::vector<std::string>& entries,
    const std::string& entry)
{
    entries.push_back(entry);
}

void addBlocker(
    std::vector<std::string>& entries,
    const std::string& entry)
{
    entries.push_back(entry);
}

} // namespace

SearchTimerWorkflowRealExecutionReadinessReviewResult
SearchTimerWorkflowRealExecutionReadinessReview::review(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerWorkflowCommandDispatchOptions& options) const
{
    SearchTimerWorkflowRealExecutionReadinessReviewResult result;

    result.planExecutable =
        plan.valid() && plan.hasExecutionWork();
    result.writeOperation =
        plan.writeOperation();
    result.executeModeRequested =
        plan.executionMode() == SearchTimerWorkflowExecutionMode::Execute;
    result.explicitOperatorConfirmationProvided =
        options.explicitOperatorConfirmation();
    result.executorOptInProvided =
        options.executorOptInEnabled();
    result.executorInjected =
        options.hasCommandExecutor();
    result.controlledTestInvocationOnly =
        options.controlledTestExecutorInvocationEnabled();
    result.productionRealExecutionPolicyAvailable = false;

    if (result.planExecutable)
    {
        addCondition(
            result.satisfiedConditions,
            "plan executable");
    }
    else
    {
        addBlocker(
            result.blockers,
            "workflow plan is not executable");
    }

    if (result.writeOperation)
    {
        addCondition(
            result.satisfiedConditions,
            "write operation requested");
    }
    else
    {
        addBlocker(
            result.blockers,
            "workflow plan is not a write operation");
    }

    if (result.executeModeRequested)
    {
        addCondition(
            result.satisfiedConditions,
            "execute mode requested");
    }
    else
    {
        addBlocker(
            result.blockers,
            "execute mode is not requested");
    }

    if (result.explicitOperatorConfirmationProvided)
    {
        addCondition(
            result.satisfiedConditions,
            "explicit operator confirmation provided");
    }
    else
    {
        addBlocker(
            result.blockers,
            "explicit operator confirmation is missing");
    }

    if (result.executorOptInProvided)
    {
        addCondition(
            result.satisfiedConditions,
            "executor opt-in provided");
    }
    else
    {
        addBlocker(
            result.blockers,
            "executor opt-in is missing");
    }

    if (result.executorInjected)
    {
        addCondition(
            result.satisfiedConditions,
            "command executor injected");
    }
    else
    {
        addBlocker(
            result.blockers,
            "command executor is not injected");
    }

    if (result.controlledTestInvocationOnly)
    {
        addCondition(
            result.satisfiedConditions,
            "controlled test invocation path available");
        addBlocker(
            result.blockers,
            "controlled test invocation is not production real execution");
    }

    if (!result.productionRealExecutionPolicyAvailable)
    {
        addBlocker(
            result.blockers,
            "production real execution policy gate is not available");
    }

    result.readyForRealBackendExecution =
        result.planExecutable &&
        result.writeOperation &&
        result.executeModeRequested &&
        result.explicitOperatorConfirmationProvided &&
        result.executorOptInProvided &&
        result.executorInjected &&
        !result.controlledTestInvocationOnly &&
        result.productionRealExecutionPolicyAvailable &&
        result.blockers.empty();

    if (result.readyForRealBackendExecution)
    {
        result.readinessStage = "real-backend-execution-ready";
        result.message = "real backend execution is ready";
    }
    else
    {
        result.readinessStage = "real-backend-execution-not-ready";
        result.message = "real backend execution is not ready";
    }

    return result;
}
