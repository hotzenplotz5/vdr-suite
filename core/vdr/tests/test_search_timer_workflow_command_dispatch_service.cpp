#include "SearchTimerWorkflowCommandDispatchService.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowCommandDispatchService dispatchService;

    const auto createPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Suche",
                "Terra X"));

    const SearchTimerWorkflowExecutionResult blockedCreate =
        dispatchService.dispatchPlan(createPlan);

    assert(!blockedCreate.success);
    assert(!blockedCreate.executed);
    assert(blockedCreate.blocked);
    assert(blockedCreate.dryRunOnly);
    assert(!blockedCreate.confirmationProvided);
    assert(blockedCreate.requiresExplicitOperatorConfirmation);
    assert(!blockedCreate.commandRequestMapped);
    assert(!blockedCreate.realExecutionEnabled);
    assert(blockedCreate.dispatchStage == "confirmation-required");
    assert(blockedCreate.executionMode == SearchTimerWorkflowExecutionMode::Prepare);
    assert(blockedCreate.operation == SearchTimerWorkflowOperation::Create);
    assert(blockedCreate.primaryStep == SearchTimerWorkflowExecutionStep::Create);
    assert(blockedCreate.hasErrors());

    const SearchTimerWorkflowExecutionResult acceptedCreate =
        dispatchService.dispatchPlan(createPlan, true);

    assert(acceptedCreate.success);
    assert(!acceptedCreate.executed);
    assert(!acceptedCreate.blocked);
    assert(acceptedCreate.dryRunOnly);
    assert(acceptedCreate.confirmationProvided);
    assert(acceptedCreate.requiresExplicitOperatorConfirmation);
    assert(acceptedCreate.requiresBackendReadback);
    assert(acceptedCreate.commandRequestMapped);
    assert(!acceptedCreate.realExecutionEnabled);
    assert(acceptedCreate.dispatchStage == "command-request-mapped");
    assert(acceptedCreate.executionMode == SearchTimerWorkflowExecutionMode::Prepare);
    assert(acceptedCreate.backendId == "home-vdr");
    assert(acceptedCreate.message == "create command request accepted by dispatch skeleton");
    assert(acceptedCreate.hasWarnings());
    assert(!acceptedCreate.hasErrors());

    const auto dryRunCreatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Dry",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::DryRun));

    const SearchTimerWorkflowExecutionResult dryRunCreate =
        dispatchService.dispatchPlan(dryRunCreatePlan, true);

    assert(dryRunCreate.success);
    assert(!dryRunCreate.executed);
    assert(!dryRunCreate.blocked);
    assert(dryRunCreate.dryRunOnly);
    assert(!dryRunCreate.commandRequestMapped);
    assert(!dryRunCreate.realExecutionEnabled);
    assert(dryRunCreate.dispatchStage == "dry-run");
    assert(dryRunCreate.executionMode == SearchTimerWorkflowExecutionMode::DryRun);
    assert(dryRunCreate.message == "write workflow accepted by dry-run execution mode");

    const auto executeCreatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Execute",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult executeCreate =
        dispatchService.dispatchPlan(executeCreatePlan, true);

    assert(!executeCreate.success);
    assert(!executeCreate.executed);
    assert(executeCreate.blocked);
    assert(executeCreate.dryRunOnly);
    assert(executeCreate.commandRequestMapped);
    assert(!executeCreate.realExecutionEnabled);
    assert(executeCreate.dispatchStage == "real-execution-disabled");
    assert(executeCreate.executionMode == SearchTimerWorkflowExecutionMode::Execute);
    assert(executeCreate.hasErrors());

    const auto updatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "remote-vdr",
                "searchtimer-42",
                "Terra X Aktualisiert",
                "Terra X neu"));

    const SearchTimerWorkflowExecutionResult acceptedUpdate =
        dispatchService.dispatchPlan(updatePlan, true);

    assert(acceptedUpdate.success);
    assert(!acceptedUpdate.executed);
    assert(!acceptedUpdate.blocked);
    assert(acceptedUpdate.dryRunOnly);
    assert(acceptedUpdate.commandRequestMapped);
    assert(!acceptedUpdate.realExecutionEnabled);
    assert(acceptedUpdate.dispatchStage == "command-request-mapped");
    assert(acceptedUpdate.requiresBackendReadback);
    assert(acceptedUpdate.backendId == "remote-vdr");
    assert(acceptedUpdate.backendNativeId == "searchtimer-42");
    assert(acceptedUpdate.operation == SearchTimerWorkflowOperation::Update);
    assert(acceptedUpdate.primaryStep == SearchTimerWorkflowExecutionStep::Update);
    assert(acceptedUpdate.followUpStep == SearchTimerWorkflowExecutionStep::Readback);
    assert(acceptedUpdate.message == "update command request accepted by dispatch skeleton");

    const auto deletePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::remove(
                "archive-vdr",
                "searchtimer-99"));

    const SearchTimerWorkflowExecutionResult acceptedDelete =
        dispatchService.dispatchPlan(deletePlan, true);

    assert(acceptedDelete.success);
    assert(!acceptedDelete.executed);
    assert(!acceptedDelete.blocked);
    assert(acceptedDelete.dryRunOnly);
    assert(acceptedDelete.confirmationProvided);
    assert(acceptedDelete.commandRequestMapped);
    assert(!acceptedDelete.realExecutionEnabled);
    assert(acceptedDelete.dispatchStage == "command-request-mapped");
    assert(!acceptedDelete.requiresBackendReadback);
    assert(acceptedDelete.backendId == "archive-vdr");
    assert(acceptedDelete.backendNativeId == "searchtimer-99");
    assert(acceptedDelete.operation == SearchTimerWorkflowOperation::Delete);
    assert(acceptedDelete.primaryStep == SearchTimerWorkflowExecutionStep::Delete);
    assert(acceptedDelete.message == "delete command request accepted by dispatch skeleton");

    const auto listPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::list());

    const SearchTimerWorkflowExecutionResult listResult =
        dispatchService.dispatchPlan(listPlan);

    assert(listResult.success);
    assert(!listResult.executed);
    assert(!listResult.blocked);
    assert(listResult.dryRunOnly);
    assert(!listResult.requiresExplicitOperatorConfirmation);
    assert(!listResult.commandRequestMapped);
    assert(!listResult.realExecutionEnabled);
    assert(listResult.dispatchStage == "read-only-no-dispatch");
    assert(listResult.operation == SearchTimerWorkflowOperation::List);
    assert(listResult.primaryStep == SearchTimerWorkflowExecutionStep::List);
    assert(listResult.message == "read-only workflow plan has no command dispatch work");
    assert(listResult.hasWarnings());

    const auto invalidUpdatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "home-vdr",
                "",
                "Terra X",
                "Terra X"));

    const SearchTimerWorkflowExecutionResult invalidResult =
        dispatchService.dispatchPlan(invalidUpdatePlan, true);

    assert(!invalidResult.success);
    assert(!invalidResult.executed);
    assert(invalidResult.blocked);
    assert(invalidResult.dryRunOnly);
    assert(!invalidResult.commandRequestMapped);
    assert(!invalidResult.realExecutionEnabled);
    assert(invalidResult.dispatchStage == "validation-blocked");
    assert(invalidResult.operation == SearchTimerWorkflowOperation::Update);
    assert(invalidResult.primaryStep == SearchTimerWorkflowExecutionStep::None);
    assert(invalidResult.hasErrors());

    std::cout
        << "test_search_timer_workflow_command_dispatch_service passed"
        << std::endl;

    return 0;
}
