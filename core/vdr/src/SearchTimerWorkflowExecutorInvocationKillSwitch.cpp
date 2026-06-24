#include "SearchTimerWorkflowExecutorInvocationKillSwitch.h"

SearchTimerWorkflowExecutorInvocationKillSwitchDecision
SearchTimerWorkflowExecutorInvocationKillSwitch::evaluate(
    const SearchTimerWorkflowGuardedExecutorInvocationDecision& guardDecision) const
{
    if (!guardDecision.guardPassed)
    {
        SearchTimerWorkflowExecutorInvocationKillSwitchDecision decision;
        decision.killSwitchOpen = open_;
        decision.allowed = false;
        decision.dispatchStage = guardDecision.dispatchStage;
        decision.message = guardDecision.message;
        decision.errors = guardDecision.errors;
        return decision;
    }

    if (!open_)
    {
        SearchTimerWorkflowExecutorInvocationKillSwitchDecision decision;
        decision.killSwitchOpen = false;
        decision.allowed = false;
        decision.dispatchStage = "executor-invocation-kill-switch-closed";
        decision.message = "executor invocation kill switch is closed";
        decision.errors = {"executor invocation kill switch is closed"};
        return decision;
    }

    SearchTimerWorkflowExecutorInvocationKillSwitchDecision decision;
    decision.killSwitchOpen = true;
    decision.allowed = true;
    decision.dispatchStage = "executor-invocation-kill-switch-open";
    decision.message = "executor invocation kill switch is open";
    decision.errors.clear();
    return decision;
}
