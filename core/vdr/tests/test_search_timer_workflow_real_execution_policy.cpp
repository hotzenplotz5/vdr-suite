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
    assert(withOptInAndExecutor.dispatchStage == "real-execution-enable-switch-required");
    assert(withOptInAndExecutor.message == "production real execution enable switch is required");
    assert(!withOptInAndExecutor.errors.empty());
    assert(executor.callCount() == 0);


    const auto withControlledTestInvocation =
        policy.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &executor));

    assert(withControlledTestInvocation.allowed);
    assert(withControlledTestInvocation.dispatchStage == "real-execution-policy-controlled-test-allowed");
    assert(withControlledTestInvocation.message == "real execution policy allows controlled test executor invocation");
    assert(withControlledTestInvocation.errors.empty());
    assert(executor.callCount() == 0);

    const SearchTimerWorkflowRealExecutionPolicyDecision productionEnabledDecision =
        policy.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabled(
                true,
                &executor));

    assert(!productionEnabledDecision.allowed);
    assert(productionEnabledDecision.dispatchStage == "backend-write-allowlist-required");
    assert(productionEnabledDecision.message == "backend write allowlist is required");
    assert(!productionEnabledDecision.errors.empty());

    const SearchTimerWorkflowRealExecutionPolicyDecision allowlistedDecision =
        policy.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlist(
                true,
                &executor,
                std::vector<std::string>{"home-vdr"}));

    assert(!allowlistedDecision.allowed);
    assert(allowlistedDecision.dispatchStage == "backend-write-permission-required");
    assert(allowlistedDecision.message == "backend write permission gate is required");
    assert(!allowlistedDecision.errors.empty());

    const SearchTimerWorkflowRealExecutionPolicyDecision permittedDecision =
        policy.evaluate(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlistAndPermission(
                true,
                &executor,
                std::vector<std::string>{"home-vdr"},
                std::vector<std::string>{"home-vdr"}));

    assert(!permittedDecision.allowed);
    assert(permittedDecision.dispatchStage == "real-execution-policy-denied");
    assert(permittedDecision.message == "real execution policy denies backend command dispatch");
    assert(!permittedDecision.errors.empty());

    std::cout
        << "test_search_timer_workflow_real_execution_policy passed"
        << std::endl;

    return 0;
}
