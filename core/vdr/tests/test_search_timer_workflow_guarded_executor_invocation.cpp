#include "SearchTimerWorkflowGuardedExecutorInvocation.h"

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

SearchTimerWorkflowRealExecutionPolicyDecision allowedPolicyDecision()
{
    SearchTimerWorkflowRealExecutionPolicyDecision decision;
    decision.allowed = true;
    decision.dispatchStage = "real-execution-policy-allowed";
    decision.message = "real execution policy allows guarded invocation";
    decision.errors.clear();
    return decision;
}

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowRealExecutionPolicy policy;
    SearchTimerWorkflowGuardedExecutorInvocation invocationGuard;

    const SearchTimerWorkflowExecutionPlan preparePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Prepare",
                "Terra X"));

    const auto prepareDecision =
        invocationGuard.evaluate(
            preparePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmed(true),
            allowedPolicyDecision(),
            true);

    assert(!prepareDecision.guardPassed);
    assert(!prepareDecision.invocationAttempted);
    assert(prepareDecision.dispatchStage == "executor-invocation-not-requested");

    const SearchTimerWorkflowExecutionPlan executePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Execute",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const auto withoutCommandRequest =
        invocationGuard.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptIn(true),
            allowedPolicyDecision(),
            false);

    assert(!withoutCommandRequest.guardPassed);
    assert(!withoutCommandRequest.invocationAttempted);
    assert(withoutCommandRequest.dispatchStage == "executor-invocation-command-request-required");

    const auto withoutOptIn =
        invocationGuard.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmed(true),
            allowedPolicyDecision(),
            true);

    assert(!withoutOptIn.guardPassed);
    assert(!withoutOptIn.invocationAttempted);
    assert(withoutOptIn.dispatchStage == "executor-opt-in-required");

    const auto withoutExecutor =
        invocationGuard.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptIn(true),
            allowedPolicyDecision(),
            true);

    assert(!withoutExecutor.guardPassed);
    assert(!withoutExecutor.invocationAttempted);
    assert(withoutExecutor.dispatchStage == "real-executor-injection-required");

    FakeSearchTimerCommandExecutor executor;

    const auto policyDenied =
        invocationGuard.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptInAndExecutor(
                true,
                &executor),
            policy.evaluate(
                executePlan,
                SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptInAndExecutor(
                    true,
                    &executor)),
            true);

    assert(!policyDenied.guardPassed);
    assert(!policyDenied.invocationAttempted);
    assert(policyDenied.dispatchStage == "real-execution-policy-denied");
    assert(executor.callCount() == 0);

    const auto ready =
        invocationGuard.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptInAndExecutor(
                true,
                &executor),
            allowedPolicyDecision(),
            true);

    assert(ready.guardPassed);
    assert(!ready.invocationAttempted);
    assert(ready.dispatchStage == "guarded-executor-invocation-ready");
    assert(ready.message == "guarded executor invocation contract is satisfied");
    assert(ready.errors.empty());
    assert(executor.callCount() == 0);

    std::cout
        << "test_search_timer_workflow_guarded_executor_invocation passed"
        << std::endl;

    return 0;
}
