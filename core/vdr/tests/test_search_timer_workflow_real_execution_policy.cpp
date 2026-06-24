#include "SearchTimerWorkflowRealExecutionPolicy.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowRealExecutionPolicy policy;

    const SearchTimerWorkflowExecutionPlan preparePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Prepare",
                "Terra X"));

    const SearchTimerWorkflowRealExecutionPolicyDecision prepareDecision =
        policy.evaluate(
            preparePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmed(true));

    assert(prepareDecision.allowed);
    assert(prepareDecision.dispatchStage == "real-execution-policy-not-required");
    assert(prepareDecision.errors.empty());

    const SearchTimerWorkflowExecutionPlan executePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Execute",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowRealExecutionPolicyDecision withoutOptIn =
        policy.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmed(true));

    assert(!withoutOptIn.allowed);
    assert(withoutOptIn.dispatchStage == "executor-opt-in-required");
    assert(withoutOptIn.message == "real execution mode requires executor opt-in");
    assert(!withoutOptIn.errors.empty());

    const SearchTimerWorkflowRealExecutionPolicyDecision withOptIn =
        policy.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptIn(true));

    assert(!withOptIn.allowed);
    assert(withOptIn.dispatchStage == "real-execution-policy-denied");
    assert(withOptIn.message == "real execution policy denies backend command dispatch");
    assert(!withOptIn.errors.empty());

    std::cout
        << "test_search_timer_workflow_real_execution_policy passed"
        << std::endl;

    return 0;
}
