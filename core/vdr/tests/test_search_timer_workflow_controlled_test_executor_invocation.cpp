#include "SearchTimerWorkflowCommandDispatchService.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class ControlledTestSearchTimerCommandExecutor : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++createCalls_;
        lastName_ = request.name;
        lastQuery_ = request.query;
        lastCompareTitle_ = request.compareTitle;
        lastCompareSubtitle_ = request.compareSubtitle;
        lastCompareSummary_ = request.compareSummary;
        lastCompareCategories_ = request.compareCategories;

        const SearchTimer created =
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    "controlled-test-vdr",
                    "controlled-created-1"),
                request.name,
                request.query,
                SearchTimerState::Active);

        return SearchTimerCreateResult::ok(
            created,
            "controlled test executor create result mapped");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        ++updateCalls_;
        lastName_ = request.name;
        lastQuery_ = request.query;
        lastCompareTitle_ = request.compareTitle;
        lastCompareSubtitle_ = request.compareSubtitle;
        lastCompareSummary_ = request.compareSummary;
        lastCompareCategories_ = request.compareCategories;

        const SearchTimer updated =
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    request.backendId,
                    request.backendNativeId),
                request.name,
                request.query,
                SearchTimerState::Active);

        return SearchTimerUpdateResult::ok(
            updated,
            "controlled test executor update result mapped");
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        ++deleteCalls_;

        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "controlled test executor delete result mapped");
    }

    int createCalls() const
    {
        return createCalls_;
    }

    int updateCalls() const
    {
        return updateCalls_;
    }

    int deleteCalls() const
    {
        return deleteCalls_;
    }

    int totalCalls() const
    {
        return createCalls_ + updateCalls_ + deleteCalls_;
    }

    const std::string& lastName() const
    {
        return lastName_;
    }

    const std::string& lastQuery() const
    {
        return lastQuery_;
    }

    bool lastCompareTitle() const
    {
        return lastCompareTitle_;
    }

    bool lastCompareSubtitle() const
    {
        return lastCompareSubtitle_;
    }

    bool lastCompareSummary() const
    {
        return lastCompareSummary_;
    }

    bool lastCompareCategories() const
    {
        return lastCompareCategories_;
    }

private:
    int createCalls_ = 0;
    int updateCalls_ = 0;
    int deleteCalls_ = 0;
    std::string lastName_;
    std::string lastQuery_;
    bool lastCompareTitle_ = false;
    bool lastCompareSubtitle_ = false;
    bool lastCompareSummary_ = false;
    bool lastCompareCategories_ = false;
};


bool hasAuditEntry(
    const std::vector<std::string>& auditTrail,
    const std::string& expected)
{
    for (const std::string& entry : auditTrail)
    {
        if (entry == expected)
        {
            return true;
        }
    }

    return false;
}

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowCommandDispatchService dispatchService;
    ControlledTestSearchTimerCommandExecutor executor;

    const SearchTimerWorkflowExecutionPlan createPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "controlled-test-vdr",
                "Amerika",
                "Amerika")
                .withCompareFields(
                    true,
                    false,
                    false,
                    false)
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult createResult =
        dispatchService.dispatchPlan(
            createPlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &executor));

    assert(createResult.success);
    assert(createResult.executed);
    assert(!createResult.blocked);
    assert(!createResult.dryRunOnly);
    assert(createResult.commandRequestMapped);
    assert(createResult.realExecutionEnabled);
    assert(createResult.realExecutionPolicyAllowed);
    assert(createResult.executorOptInProvided);
    assert(createResult.executorInjected);
    assert(createResult.executorInvocationGuardPassed);
    assert(createResult.executorInvocationAttempted);
    assert(createResult.executorInvocationKillSwitchOpen);
    assert(createResult.executorInvocationKillSwitchPassed);
    assert(createResult.executorResultMapped);
    assert(createResult.executorResultSuccessful);
    assert(createResult.dispatchStage == "executor-result-mapped");
    assert(createResult.backendId == "controlled-test-vdr");
    assert(createResult.backendNativeId == "controlled-created-1");
    assert(createResult.message == "controlled test executor create result mapped");
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "controlledTestExecutorInvocation=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "executorResultSuccessful=true"));
    assert(executor.createCalls() == 1);
    assert(executor.updateCalls() == 0);
    assert(executor.deleteCalls() == 0);
    assert(executor.lastName() == "Amerika");
    assert(executor.lastQuery() == "Amerika");
    assert(executor.lastCompareTitle());
    assert(!executor.lastCompareSubtitle());
    assert(!executor.lastCompareSummary());
    assert(!executor.lastCompareCategories());

    const SearchTimerWorkflowExecutionPlan updatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "controlled-test-vdr",
                "controlled-updated-7",
                "Amerika",
                "Amerika")
                .withCompareFields(
                    true,
                    false,
                    false,
                    false)
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult updateResult =
        dispatchService.dispatchPlan(
            updatePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &executor));

    assert(updateResult.success);
    assert(updateResult.executed);
    assert(updateResult.executorResultMapped);
    assert(updateResult.executorResultSuccessful);
    assert(updateResult.backendId == "controlled-test-vdr");
    assert(updateResult.backendNativeId == "controlled-updated-7");
    assert(updateResult.message == "controlled test executor update result mapped");
    assert(executor.createCalls() == 1);
    assert(executor.updateCalls() == 1);
    assert(executor.deleteCalls() == 0);
    assert(executor.lastName() == "Amerika");
    assert(executor.lastQuery() == "Amerika");
    assert(executor.lastCompareTitle());
    assert(!executor.lastCompareSubtitle());
    assert(!executor.lastCompareSummary());
    assert(!executor.lastCompareCategories());

    const SearchTimerWorkflowExecutionPlan deletePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::remove(
                "controlled-test-vdr",
                "controlled-deleted-9")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult deleteResult =
        dispatchService.dispatchPlan(
            deletePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &executor));

    assert(deleteResult.success);
    assert(deleteResult.executed);
    assert(deleteResult.executorResultMapped);
    assert(deleteResult.executorResultSuccessful);
    assert(deleteResult.backendId == "controlled-test-vdr");
    assert(deleteResult.backendNativeId == "controlled-deleted-9");
    assert(deleteResult.message == "controlled test executor delete result mapped");
    assert(executor.totalCalls() == 3);

    std::cout
        << "test_search_timer_workflow_controlled_test_executor_invocation passed"
        << std::endl;

    return 0;
}
