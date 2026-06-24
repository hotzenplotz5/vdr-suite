#include "SearchTimerWorkflowRealExecutionPolicy.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>

class FakeSearchTimerCommandExecutor : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        (void)request;
        ++createCalls_;
        return SearchTimerCreateResult::failed(
            "unexpected create call",
            {"unexpected create call"});
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        (void)request;
        ++updateCalls_;
        return SearchTimerUpdateResult::failed(
            "unexpected update call",
            {"unexpected update call"});
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        (void)request;
        ++deleteCalls_;
        return SearchTimerDeleteResult::failed(
            "unexpected delete call",
            {"unexpected delete call"});
    }

    int callCount() const
    {
        return createCalls_ + updateCalls_ + deleteCalls_;
    }

private:
    int createCalls_ = 0;
    int updateCalls_ = 0;
    int deleteCalls_ = 0;
};

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

    const SearchTimerWorkflowRealExecutionPolicyDecision withOptInWithoutExecutor =
        policy.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptIn(true));

    assert(!withOptInWithoutExecutor.allowed);
    assert(withOptInWithoutExecutor.dispatchStage == "real-executor-injection-required");
    assert(withOptInWithoutExecutor.message == "real execution mode requires an injected command executor");
    assert(!withOptInWithoutExecutor.errors.empty());

    FakeSearchTimerCommandExecutor executor;

    const SearchTimerWorkflowRealExecutionPolicyDecision withOptInAndExecutor =
        policy.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptInAndExecutor(
                true,
                &executor));

    assert(!withOptInAndExecutor.allowed);
    assert(withOptInAndExecutor.dispatchStage == "real-execution-policy-denied");
    assert(withOptInAndExecutor.message == "real execution policy denies backend command dispatch");
    assert(!withOptInAndExecutor.errors.empty());
    assert(executor.callCount() == 0);

    std::cout
        << "test_search_timer_workflow_real_execution_policy passed"
        << std::endl;

    return 0;
}
