#include "SearchTimerWorkflowRealExecutionPolicy.h"

#include "SearchTimerWorkflowBackendWriteAllowlist.h"

SearchTimerWorkflowRealExecutionPolicyDecision
SearchTimerWorkflowRealExecutionPolicy::evaluate(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerWorkflowCommandDispatchOptions& options) const
{
    if (plan.executionMode() != SearchTimerWorkflowExecutionMode::Execute)
    {
        SearchTimerWorkflowRealExecutionPolicyDecision decision;
        decision.allowed = true;
        decision.dispatchStage = "real-execution-policy-not-required";
        decision.message = "real execution policy is not required for non-execute mode";
        decision.errors.clear();
        return decision;
    }

    if (!options.executorOptInEnabled())
    {
        SearchTimerWorkflowRealExecutionPolicyDecision decision;
        decision.allowed = false;
        decision.dispatchStage = "executor-opt-in-required";
        decision.message = "real execution mode requires executor opt-in";
        decision.errors = {"real execution mode requires executor opt-in"};
        return decision;
    }

    if (!options.hasCommandExecutor())
    {
        SearchTimerWorkflowRealExecutionPolicyDecision decision;
        decision.allowed = false;
        decision.dispatchStage = "real-executor-injection-required";
        decision.message = "real execution mode requires an injected command executor";
        decision.errors = {"real execution mode requires an injected command executor"};
        return decision;
    }

    if (options.controlledTestExecutorInvocationEnabled())
    {
        SearchTimerWorkflowRealExecutionPolicyDecision decision;
        decision.allowed = true;
        decision.dispatchStage = "real-execution-policy-controlled-test-allowed";
        decision.message = "real execution policy allows controlled test executor invocation";
        decision.errors.clear();
        return decision;
    }

    if (!options.productionRealExecutionEnabled())
    {
        SearchTimerWorkflowRealExecutionPolicyDecision decision;
        decision.allowed = false;
        decision.dispatchStage = "real-execution-enable-switch-required";
        decision.message = "production real execution enable switch is required";
        decision.errors = {"production real execution enable switch is required"};
        return decision;
    }

    const SearchTimerWorkflowBackendWriteAllowlistDecision allowlistDecision =
        SearchTimerWorkflowBackendWriteAllowlist().evaluate(
            plan,
            options);

    if (!allowlistDecision.allowed)
    {
        SearchTimerWorkflowRealExecutionPolicyDecision decision;
        decision.allowed = false;
        decision.dispatchStage = allowlistDecision.dispatchStage;
        decision.message = allowlistDecision.message;
        decision.errors = allowlistDecision.errors;
        return decision;
    }

    SearchTimerWorkflowRealExecutionPolicyDecision decision;
    decision.allowed = false;
    decision.dispatchStage = "real-execution-policy-denied";
    decision.message = "real execution policy denies backend command dispatch";
    decision.errors = {"real execution policy denies backend command dispatch"};
    return decision;
}
