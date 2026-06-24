#include "SearchTimerWorkflowExecutionService.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowExecutionService executionService;

    const auto listPlan =
        planningService.plan(SearchTimerWorkflowRequest::list());
    const auto listResult =
        executionService.executePlan(listPlan);

    assert(listResult.success);
    assert(!listResult.executed);
    assert(!listResult.blocked);
    assert(listResult.dryRunOnly);
    assert(!listResult.confirmationProvided);
    assert(!listResult.requiresExplicitOperatorConfirmation);
    assert(!listResult.requiresBackendReadback);
    assert(listResult.operation == SearchTimerWorkflowOperation::List);
    assert(listResult.primaryStep == SearchTimerWorkflowExecutionStep::List);
    assert(!listResult.hasErrors());
    assert(listResult.hasWarnings());

    const auto createPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Suche",
                "Terra X"));

    const auto blockedCreate =
        executionService.executePlan(createPlan);

    assert(!blockedCreate.success);
    assert(!blockedCreate.executed);
    assert(blockedCreate.blocked);
    assert(blockedCreate.dryRunOnly);
    assert(!blockedCreate.confirmationProvided);
    assert(blockedCreate.requiresExplicitOperatorConfirmation);
    assert(blockedCreate.requiresBackendReadback);
    assert(blockedCreate.operation == SearchTimerWorkflowOperation::Create);
    assert(blockedCreate.primaryStep == SearchTimerWorkflowExecutionStep::Create);
    assert(blockedCreate.followUpStep == SearchTimerWorkflowExecutionStep::Readback);
    assert(blockedCreate.hasErrors());

    const auto acceptedCreate =
        executionService.executePlan(createPlan, true);

    assert(acceptedCreate.success);
    assert(!acceptedCreate.executed);
    assert(!acceptedCreate.blocked);
    assert(acceptedCreate.dryRunOnly);
    assert(acceptedCreate.confirmationProvided);
    assert(acceptedCreate.requiresExplicitOperatorConfirmation);
    assert(acceptedCreate.requiresBackendReadback);
    assert(acceptedCreate.backendId == "home-vdr");
    assert(acceptedCreate.hasWarnings());
    assert(!acceptedCreate.hasErrors());

    const auto deletePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::remove(
                "home-vdr",
                "searchtimer-42"));

    const auto deleteResult =
        executionService.executePlan(deletePlan, true);

    assert(deleteResult.success);
    assert(!deleteResult.executed);
    assert(!deleteResult.blocked);
    assert(deleteResult.confirmationProvided);
    assert(deleteResult.requiresExplicitOperatorConfirmation);
    assert(!deleteResult.requiresBackendReadback);
    assert(deleteResult.operation == SearchTimerWorkflowOperation::Delete);
    assert(deleteResult.primaryStep == SearchTimerWorkflowExecutionStep::Delete);
    assert(deleteResult.followUpStep == SearchTimerWorkflowExecutionStep::None);
    assert(deleteResult.backendNativeId == "searchtimer-42");

    const auto invalidPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "",
                "Terra X Suche",
                "Terra X"));
    const auto invalidResult =
        executionService.executePlan(invalidPlan);

    assert(!invalidResult.success);
    assert(!invalidResult.executed);
    assert(invalidResult.blocked);
    assert(invalidResult.dryRunOnly);
    assert(invalidResult.operation == SearchTimerWorkflowOperation::Update);
    assert(invalidResult.primaryStep == SearchTimerWorkflowExecutionStep::None);
    assert(invalidResult.hasErrors());

    std::cout
        << "test_search_timer_workflow_execution_service passed"
        << std::endl;

    return 0;
}
