#include "SearchTimerWorkflowGuardedExecutorInvocation.h"

SearchTimerWorkflowGuardedExecutorInvocationDecision
SearchTimerWorkflowGuardedExecutorInvocation::evaluate(
    const SearchTimerWorkflowExecutionPlan& plan,
    const SearchTimerWorkflowCommandDispatchOptions& options,
    const SearchTimerWorkflowRealExecutionPolicyDecision& policyDecision,
    bool commandRequestMapped) const
{
    if (plan.executionMode() != SearchTimerWorkflowExecutionMode::Execute)
    {
        SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
        decision.guardPassed = false;
        decision.invocationAttempted = false;
        decision.dispatchStage = "executor-invocation-not-requested";
        decision.message = "executor invocation is not requested for non-execute mode";
        decision.errors = {
            "executor invocation is not requested for non-execute mode"};
        return decision;
    }

    if (!plan.writeOperation())
    {
        SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
        decision.guardPassed = false;
        decision.invocationAttempted = false;
        decision.dispatchStage = "executor-invocation-not-write-operation";
        decision.message = "executor invocation requires a write workflow plan";
        decision.errors = {
            "executor invocation requires a write workflow plan"};
        return decision;
    }

    if (!commandRequestMapped)
    {
        SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
        decision.guardPassed = false;
        decision.invocationAttempted = false;
        decision.dispatchStage = "executor-invocation-command-request-required";
        decision.message = "executor invocation requires a mapped command request";
        decision.errors = {
            "executor invocation requires a mapped command request"};
        return decision;
    }

    if (!options.executorOptInEnabled())
    {
        SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
        decision.guardPassed = false;
        decision.invocationAttempted = false;
        decision.dispatchStage = "executor-opt-in-required";
        decision.message = "real execution mode requires executor opt-in";
        decision.errors = {"real execution mode requires executor opt-in"};
        return decision;
    }

    if (!options.hasCommandExecutor())
    {
        SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
        decision.guardPassed = false;
        decision.invocationAttempted = false;
        decision.dispatchStage = "real-executor-injection-required";
        decision.message = "real execution mode requires an injected command executor";
        decision.errors = {
            "real execution mode requires an injected command executor"};
        return decision;
    }

    if (!policyDecision.allowed)
    {
        SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
        decision.guardPassed = false;
        decision.invocationAttempted = false;
        decision.dispatchStage = policyDecision.dispatchStage;
        decision.message = policyDecision.message;
        decision.errors = policyDecision.errors;
        return decision;
    }

    SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
    decision.guardPassed = true;
    decision.invocationAttempted = false;
    decision.dispatchStage = "guarded-executor-invocation-ready";
    decision.message = "guarded executor invocation contract is satisfied";
    decision.errors.clear();
    return decision;
}
