#include "SearchTimerWorkflowRealExecutionReadinessReview.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>

class ReadinessReviewCommandExecutor : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest&) override
    {
        ++callCount_;
        return SearchTimerCreateResult::failed(
            "unexpected create call",
            {"unexpected create call"});
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest&) override
    {
        ++callCount_;
        return SearchTimerUpdateResult::failed(
            "unexpected update call",
            {"unexpected update call"});
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest&) override
    {
        ++callCount_;
        return SearchTimerDeleteResult::failed(
            "unexpected delete call",
            {"unexpected delete call"});
    }

    int callCount() const
    {
        return callCount_;
    }

private:
    int callCount_ = 0;
};

bool contains(
    const std::vector<std::string>& values,
    const std::string& expected)
{
    for (const std::string& value : values)
    {
        if (value == expected)
        {
            return true;
        }
    }

    return false;
}

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowRealExecutionReadinessReview review;
    ReadinessReviewCommandExecutor executor;

    const SearchTimerWorkflowExecutionPlan preparePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Prepare",
                "Terra X"));

    const SearchTimerWorkflowRealExecutionReadinessReviewResult prepareResult =
        review.review(
            preparePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmed(
                true));

    assert(!prepareResult.readyForRealBackendExecution);
    assert(prepareResult.planExecutable);
    assert(prepareResult.writeOperation);
    assert(!prepareResult.executeModeRequested);
    assert(prepareResult.explicitOperatorConfirmationProvided);
    assert(!prepareResult.executorOptInProvided);
    assert(!prepareResult.executorInjected);
    assert(!prepareResult.controlledTestInvocationOnly);
    assert(!prepareResult.productionRealExecutionEnabled);
    assert(!prepareResult.productionRealExecutionPolicyAvailable);
    assert(prepareResult.readinessStage == "real-backend-execution-not-ready");
    assert(prepareResult.hasBlockers());
    assert(contains(
        prepareResult.blockers,
        "execute mode is not requested"));
    assert(contains(
        prepareResult.blockers,
        "executor opt-in is missing"));
    assert(contains(
        prepareResult.blockers,
        "command executor is not injected"));
    assert(contains(
        prepareResult.blockers,
        "production real execution policy gate is not available"));

    const SearchTimerWorkflowExecutionPlan executePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Execute",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowRealExecutionReadinessReviewResult productionLikeResult =
        review.review(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptInAndExecutor(
                true,
                &executor));

    assert(!productionLikeResult.readyForRealBackendExecution);
    assert(productionLikeResult.planExecutable);
    assert(productionLikeResult.writeOperation);
    assert(productionLikeResult.executeModeRequested);
    assert(productionLikeResult.explicitOperatorConfirmationProvided);
    assert(productionLikeResult.executorOptInProvided);
    assert(productionLikeResult.executorInjected);
    assert(!productionLikeResult.controlledTestInvocationOnly);
    assert(!productionLikeResult.productionRealExecutionEnabled);
    assert(!productionLikeResult.productionRealExecutionPolicyAvailable);
    assert(productionLikeResult.hasBlockers());
    assert(contains(
        productionLikeResult.satisfiedConditions,
        "command executor injected"));
    assert(contains(
        productionLikeResult.blockers,
        "production real execution policy gate is not available"));
    assert(executor.callCount() == 0);

    const SearchTimerWorkflowRealExecutionReadinessReviewResult controlledTestResult =
        review.review(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &executor));

    assert(!controlledTestResult.readyForRealBackendExecution);
    assert(controlledTestResult.planExecutable);
    assert(controlledTestResult.writeOperation);
    assert(controlledTestResult.executeModeRequested);
    assert(controlledTestResult.executorOptInProvided);
    assert(controlledTestResult.executorInjected);
    assert(controlledTestResult.controlledTestInvocationOnly);
    assert(!controlledTestResult.productionRealExecutionEnabled);
    assert(!controlledTestResult.productionRealExecutionPolicyAvailable);
    assert(contains(
        controlledTestResult.satisfiedConditions,
        "controlled test invocation path available"));
    assert(contains(
        controlledTestResult.blockers,
        "controlled test invocation is not production real execution"));
    assert(contains(
        controlledTestResult.blockers,
        "production real execution policy gate is not available"));
    assert(executor.callCount() == 0);

    const SearchTimerWorkflowRealExecutionReadinessReviewResult productionEnabledResult =
        review.review(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabled(
                true,
                &executor));

    assert(!productionEnabledResult.readyForRealBackendExecution);
    assert(productionEnabledResult.planExecutable);
    assert(productionEnabledResult.writeOperation);
    assert(productionEnabledResult.executeModeRequested);
    assert(productionEnabledResult.executorOptInProvided);
    assert(productionEnabledResult.executorInjected);
    assert(!productionEnabledResult.controlledTestInvocationOnly);
    assert(productionEnabledResult.productionRealExecutionEnabled);
    assert(!productionEnabledResult.backendWriteAllowlistConfigured);
    assert(!productionEnabledResult.backendWriteAllowed);
    assert(!productionEnabledResult.productionRealExecutionPolicyAvailable);
    assert(contains(
        productionEnabledResult.satisfiedConditions,
        "production real execution enable switch is enabled"));
    assert(contains(
        productionEnabledResult.blockers,
        "backend write allowlist is not configured"));
    assert(contains(
        productionEnabledResult.blockers,
        "backend write allowlist is required"));
    assert(contains(
        productionEnabledResult.blockers,
        "production real execution policy gate is not available"));

    const SearchTimerWorkflowRealExecutionReadinessReviewResult allowlistedResult =
        review.review(
            executePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlist(
                true,
                &executor,
                std::vector<std::string>{"home-vdr"}));

    assert(!allowlistedResult.readyForRealBackendExecution);
    assert(allowlistedResult.productionRealExecutionEnabled);
    assert(allowlistedResult.backendWriteAllowlistConfigured);
    assert(allowlistedResult.backendWriteAllowed);
    assert(!allowlistedResult.productionRealExecutionPolicyAvailable);
    assert(contains(
        allowlistedResult.satisfiedConditions,
        "backend write allowlist is configured"));
    assert(contains(
        allowlistedResult.satisfiedConditions,
        "backend write is allowed"));
    assert(contains(
        allowlistedResult.blockers,
        "production real execution policy gate is not available"));
    assert(executor.callCount() == 0);

    std::cout
        << "test_search_timer_workflow_real_execution_readiness_review passed"
        << std::endl;

    return 0;
}
