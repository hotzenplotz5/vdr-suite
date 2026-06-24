#include "SearchTimerWorkflowRealExecutionPolicy.h"

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

    SearchTimerWorkflowRealExecutionPolicyDecision decision;
    decision.allowed = false;
    decision.dispatchStage = "real-execution-policy-denied";
    decision.message = "real execution policy denies backend command dispatch";
    decision.errors = {"real execution policy denies backend command dispatch"};
    return decision;
}
