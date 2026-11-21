#include "SearchTimerWorkflowExecutorInvocationKillSwitch.h"

#include <cassert>
#include <iostream>

SearchTimerWorkflowGuardedExecutorInvocationDecision blockedGuardDecision()
{
    SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
    decision.guardPassed = false;
    decision.invocationAttempted = false;
    decision.dispatchStage = "real-execution-policy-denied";
    decision.message = "real execution policy denies backend command dispatch";
    decision.errors = {"real execution policy denies backend command dispatch"};
    return decision;
}

SearchTimerWorkflowGuardedExecutorInvocationDecision readyGuardDecision()
{
    SearchTimerWorkflowGuardedExecutorInvocationDecision decision;
    decision.guardPassed = true;
    decision.invocationAttempted = false;
    decision.dispatchStage = "guarded-executor-invocation-ready";
    decision.message = "guarded executor invocation contract is satisfied";
    decision.errors.clear();
    return decision;
}

int main()
{
    const SearchTimerWorkflowExecutorInvocationKillSwitch closed =
        SearchTimerWorkflowExecutorInvocationKillSwitch::closed();

    const SearchTimerWorkflowExecutorInvocationKillSwitchDecision blockedByGuard =
        closed.evaluate(blockedGuardDecision());

    assert(!blockedByGuard.allowed);
    assert(!blockedByGuard.killSwitchOpen);
    assert(blockedByGuard.dispatchStage == "real-execution-policy-denied");
    assert(!blockedByGuard.errors.empty());

    const SearchTimerWorkflowExecutorInvocationKillSwitchDecision closedDecision =
        closed.evaluate(readyGuardDecision());

    assert(!closedDecision.allowed);
    assert(!closedDecision.killSwitchOpen);
    assert(closedDecision.dispatchStage == "executor-invocation-kill-switch-closed");
    assert(closedDecision.message == "executor invocation kill switch is closed");
    assert(!closedDecision.errors.empty());

    const SearchTimerWorkflowExecutorInvocationKillSwitch opened =
        SearchTimerWorkflowExecutorInvocationKillSwitch::openedForControlledExecution();

    const SearchTimerWorkflowExecutorInvocationKillSwitchDecision openedDecision =
        opened.evaluate(readyGuardDecision());

    assert(openedDecision.allowed);
    assert(openedDecision.killSwitchOpen);
    assert(openedDecision.dispatchStage == "executor-invocation-kill-switch-open");
    assert(openedDecision.message == "executor invocation kill switch is open");
    assert(openedDecision.errors.empty());

    std::cout
        << "test_search_timer_workflow_executor_invocation_kill_switch passed"
        << std::endl;

    return 0;
}
