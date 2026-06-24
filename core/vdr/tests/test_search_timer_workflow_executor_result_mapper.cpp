#include "SearchTimerWorkflowExecutorResultMapper.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowExecutorResultMapper mapper;

    const SearchTimerWorkflowExecutionPlan createPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Create",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimer created =
        SearchTimer::create(
            SearchTimerId::fromBackendNativeId(
                "home-vdr",
                "created-42"),
            "Terra X Create",
            "Terra X",
            SearchTimerState::Active);

    const SearchTimerWorkflowExecutionResult mappedCreate =
        mapper.mapCreateResult(
            createPlan,
            SearchTimerCreateResult::ok(
                created,
                "create executor result mapped"));

    assert(mappedCreate.success);
    assert(mappedCreate.executed);
    assert(!mappedCreate.blocked);
    assert(!mappedCreate.dryRunOnly);
    assert(mappedCreate.commandRequestMapped);
    assert(mappedCreate.realExecutionEnabled);
    assert(mappedCreate.realExecutionPolicyAllowed);
    assert(mappedCreate.executorOptInProvided);
    assert(mappedCreate.executorInjected);
    assert(mappedCreate.executorInvocationGuardPassed);
    assert(mappedCreate.executorInvocationAttempted);
    assert(mappedCreate.executorInvocationKillSwitchOpen);
    assert(mappedCreate.executorInvocationKillSwitchPassed);
    assert(mappedCreate.executorResultMapped);
    assert(mappedCreate.executorResultSuccessful);
    assert(mappedCreate.dispatchStage == "executor-result-mapped");
    assert(!mappedCreate.executorInvocationAuditTrail.empty());
    assert(mappedCreate.backendId == "home-vdr");
    assert(mappedCreate.backendNativeId == "created-42");
    assert(mappedCreate.message == "create executor result mapped");

    const SearchTimerWorkflowExecutionPlan updatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "searchtimer-7",
                "Terra X Update",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult mappedUpdateFailure =
        mapper.mapUpdateResult(
            updatePlan,
            SearchTimerUpdateResult::failed(
                "update executor result failed",
                {"backend rejected update"}));

    assert(!mappedUpdateFailure.success);
    assert(!mappedUpdateFailure.executed);
    assert(mappedUpdateFailure.blocked);
    assert(!mappedUpdateFailure.dryRunOnly);
    assert(mappedUpdateFailure.commandRequestMapped);
    assert(mappedUpdateFailure.realExecutionEnabled);
    assert(mappedUpdateFailure.realExecutionPolicyAllowed);
    assert(mappedUpdateFailure.executorInvocationGuardPassed);
    assert(mappedUpdateFailure.executorInvocationAttempted);
    assert(mappedUpdateFailure.executorInvocationKillSwitchOpen);
    assert(mappedUpdateFailure.executorInvocationKillSwitchPassed);
    assert(mappedUpdateFailure.executorResultMapped);
    assert(!mappedUpdateFailure.executorResultSuccessful);
    assert(mappedUpdateFailure.dispatchStage == "executor-result-failed");
    assert(!mappedUpdateFailure.executorInvocationAuditTrail.empty());
    assert(mappedUpdateFailure.message == "update executor result failed");
    assert(mappedUpdateFailure.hasErrors());

    const SearchTimerWorkflowExecutionPlan deletePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::remove(
                "archive-vdr",
                "delete-99")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult mappedDelete =
        mapper.mapDeleteResult(
            deletePlan,
            SearchTimerDeleteResult::ok(
                "archive-vdr",
                "delete-99",
                "delete executor result mapped"));

    assert(mappedDelete.success);
    assert(mappedDelete.executed);
    assert(!mappedDelete.blocked);
    assert(!mappedDelete.dryRunOnly);
    assert(mappedDelete.executorResultMapped);
    assert(mappedDelete.executorResultSuccessful);
    assert(mappedDelete.dispatchStage == "executor-result-mapped");
    assert(mappedDelete.backendId == "archive-vdr");
    assert(mappedDelete.backendNativeId == "delete-99");

    std::cout
        << "test_search_timer_workflow_executor_result_mapper passed"
        << std::endl;

    return 0;
}
