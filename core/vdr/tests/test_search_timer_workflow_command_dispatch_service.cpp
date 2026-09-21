#include "SearchTimerWorkflowCommandDispatchService.h"

#include "ISearchTimerDataSource.h"
#include "SearchTimer.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"
#include "SearchTimerService.h"
#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

SearchTimer makeTimer(
    const std::string& backendId,
    const std::string& nativeId,
    const std::string& name,
    const std::string& query,
    SearchTimerState state)
{
    return SearchTimer::create(
        SearchTimerId::fromBackendNativeId(
            backendId,
            nativeId),
        name,
        query,
        state);
}

class StaticSearchTimerDataSource final : public ISearchTimerDataSource
{
public:
    explicit StaticSearchTimerDataSource(
        std::vector<SearchTimer> timers)
        : timers_(std::move(timers))
    {
    }

    SearchTimerResult list(
        const SearchTimerQuery& query) const override
    {
        return service_.list(
            timers_,
            query);
    }

private:
    std::vector<SearchTimer> timers_;
    SearchTimerService service_;
};

class SuccessfulSearchTimerCommandExecutor : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++createCalls_;
        return SearchTimerCreateResult::ok(
            makeTimer(
                request.backendId,
                "created-native-1",
                request.name,
                request.query,
                request.active
                    ? SearchTimerState::Active
                    : SearchTimerState::Inactive),
            "create ok");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        ++updateCalls_;
        return SearchTimerUpdateResult::ok(
            makeTimer(
                request.backendId,
                request.backendNativeId,
                request.name,
                request.query,
                request.active
                    ? SearchTimerState::Active
                    : SearchTimerState::Inactive),
            "update ok");
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        ++deleteCalls_;
        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "delete ok");
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

private:
    int createCalls_ = 0;
    int updateCalls_ = 0;
    int deleteCalls_ = 0;
};

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
    assert(!blockedCreate.realExecutionPolicyAllowed);
    assert(!blockedCreate.executorOptInProvided);
    assert(!blockedCreate.executorInjected);
    assert(!blockedCreate.executorInvocationGuardPassed);
    assert(!blockedCreate.executorResultMapped);
    assert(!blockedCreate.executorResultSuccessful);
    assert(!blockedCreate.executorInvocationAttempted);
    assert(!blockedCreate.executorInvocationKillSwitchOpen);
    assert(!blockedCreate.executorInvocationKillSwitchPassed);
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
    assert(!acceptedCreate.realExecutionPolicyAllowed);
    assert(!acceptedCreate.executorOptInProvided);
    assert(!acceptedCreate.executorInjected);
    assert(!acceptedCreate.executorInvocationGuardPassed);
    assert(!acceptedCreate.executorResultMapped);
    assert(!acceptedCreate.executorResultSuccessful);
    assert(!acceptedCreate.executorInvocationAttempted);
    assert(!acceptedCreate.executorInvocationKillSwitchOpen);
    assert(!acceptedCreate.executorInvocationKillSwitchPassed);
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
    assert(!dryRunCreate.realExecutionPolicyAllowed);
    assert(!dryRunCreate.executorOptInProvided);
    assert(!dryRunCreate.executorInjected);
    assert(!dryRunCreate.executorInvocationGuardPassed);
    assert(!dryRunCreate.executorResultMapped);
    assert(!dryRunCreate.executorResultSuccessful);
    assert(!dryRunCreate.executorInvocationAttempted);
    assert(!dryRunCreate.executorInvocationKillSwitchOpen);
    assert(!dryRunCreate.executorInvocationKillSwitchPassed);
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

    const SearchTimerWorkflowExecutionResult executeCreateWithoutOptIn =
        dispatchService.dispatchPlan(executeCreatePlan, true);

    assert(!executeCreateWithoutOptIn.success);
    assert(!executeCreateWithoutOptIn.executed);
    assert(executeCreateWithoutOptIn.blocked);
    assert(executeCreateWithoutOptIn.dryRunOnly);
    assert(executeCreateWithoutOptIn.commandRequestMapped);
    assert(!executeCreateWithoutOptIn.realExecutionEnabled);
    assert(!executeCreateWithoutOptIn.realExecutionPolicyAllowed);
    assert(!executeCreateWithoutOptIn.executorOptInProvided);
    assert(!executeCreateWithoutOptIn.executorInjected);
    assert(!executeCreateWithoutOptIn.executorInvocationGuardPassed);
    assert(!executeCreateWithoutOptIn.executorResultMapped);
    assert(!executeCreateWithoutOptIn.executorResultSuccessful);
    assert(!executeCreateWithoutOptIn.executorInvocationAttempted);
    assert(!executeCreateWithoutOptIn.executorInvocationKillSwitchOpen);
    assert(!executeCreateWithoutOptIn.executorInvocationKillSwitchPassed);
    assert(executeCreateWithoutOptIn.dispatchStage == "executor-opt-in-required");
    assert(executeCreateWithoutOptIn.executionMode == SearchTimerWorkflowExecutionMode::Execute);
    assert(executeCreateWithoutOptIn.hasErrors());

    const SearchTimerWorkflowExecutionResult executeCreateWithOptIn =
        dispatchService.dispatchPlan(
            executeCreatePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptIn(true));

    assert(!executeCreateWithOptIn.success);
    assert(!executeCreateWithOptIn.executed);
    assert(executeCreateWithOptIn.blocked);
    assert(executeCreateWithOptIn.dryRunOnly);
    assert(executeCreateWithOptIn.commandRequestMapped);
    assert(!executeCreateWithOptIn.realExecutionEnabled);
    assert(!executeCreateWithOptIn.realExecutionPolicyAllowed);
    assert(executeCreateWithOptIn.executorOptInProvided);
    assert(!executeCreateWithOptIn.executorInjected);
    assert(!executeCreateWithOptIn.executorInvocationGuardPassed);
    assert(!executeCreateWithOptIn.executorResultMapped);
    assert(!executeCreateWithOptIn.executorResultSuccessful);
    assert(!executeCreateWithOptIn.executorInvocationAttempted);
    assert(!executeCreateWithOptIn.executorInvocationKillSwitchOpen);
    assert(!executeCreateWithOptIn.executorInvocationKillSwitchPassed);
    assert(executeCreateWithOptIn.dispatchStage == "real-executor-injection-required");
    assert(executeCreateWithOptIn.message == "real execution mode requires an injected command executor");
    assert(executeCreateWithOptIn.executionMode == SearchTimerWorkflowExecutionMode::Execute);
    assert(executeCreateWithOptIn.hasErrors());

    FakeSearchTimerCommandExecutor injectedExecutor;

    const SearchTimerWorkflowExecutionResult executeCreateWithInjectedExecutor =
        dispatchService.dispatchPlan(
            executeCreatePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptInAndExecutor(
                true,
                &injectedExecutor));

    assert(!executeCreateWithInjectedExecutor.success);
    assert(!executeCreateWithInjectedExecutor.executed);
    assert(executeCreateWithInjectedExecutor.blocked);
    assert(executeCreateWithInjectedExecutor.dryRunOnly);
    assert(executeCreateWithInjectedExecutor.commandRequestMapped);
    assert(!executeCreateWithInjectedExecutor.realExecutionEnabled);
    assert(!executeCreateWithInjectedExecutor.realExecutionPolicyAllowed);
    assert(executeCreateWithInjectedExecutor.executorOptInProvided);
    assert(executeCreateWithInjectedExecutor.executorInjected);
    assert(!executeCreateWithInjectedExecutor.executorInvocationGuardPassed);
    assert(!executeCreateWithInjectedExecutor.executorResultMapped);
    assert(!executeCreateWithInjectedExecutor.executorResultSuccessful);
    assert(!executeCreateWithInjectedExecutor.executorInvocationAttempted);
    assert(!executeCreateWithInjectedExecutor.executorInvocationKillSwitchOpen);
    assert(!executeCreateWithInjectedExecutor.executorInvocationKillSwitchPassed);
    assert(executeCreateWithInjectedExecutor.dispatchStage == "real-execution-enable-switch-required");
    assert(executeCreateWithInjectedExecutor.message == "production real execution enable switch is required");
    assert(executeCreateWithInjectedExecutor.executionMode == SearchTimerWorkflowExecutionMode::Execute);
    assert(executeCreateWithInjectedExecutor.hasErrors());
    assert(injectedExecutor.callCount() == 0);

    FakeSearchTimerCommandExecutor controlledExecutor;

    const SearchTimerWorkflowExecutionResult controlledCreate =
        dispatchService.dispatchPlan(
            executeCreatePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &controlledExecutor));

    assert(!controlledCreate.success);
    assert(!controlledCreate.executed);
    assert(controlledCreate.blocked);
    assert(!controlledCreate.dryRunOnly);
    assert(controlledCreate.commandRequestMapped);
    assert(controlledCreate.realExecutionEnabled);
    assert(controlledCreate.realExecutionPolicyAllowed);
    assert(controlledCreate.executorOptInProvided);
    assert(controlledCreate.executorInjected);
    assert(controlledCreate.executorInvocationGuardPassed);
    assert(controlledCreate.executorInvocationAttempted);
    assert(controlledCreate.executorInvocationKillSwitchOpen);
    assert(controlledCreate.executorInvocationKillSwitchPassed);
    assert(controlledCreate.executorResultMapped);
    assert(!controlledCreate.executorResultSuccessful);
    assert(controlledCreate.dispatchStage == "executor-result-failed");
    assert(controlledCreate.message == "unexpected create call");
    assert(controlledExecutor.callCount() == 1);


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
    assert(!acceptedUpdate.realExecutionPolicyAllowed);
    assert(!acceptedUpdate.executorOptInProvided);
    assert(!acceptedUpdate.executorInjected);
    assert(!acceptedUpdate.executorInvocationGuardPassed);
    assert(!acceptedUpdate.executorResultMapped);
    assert(!acceptedUpdate.executorResultSuccessful);
    assert(!acceptedUpdate.executorInvocationAttempted);
    assert(!acceptedUpdate.executorInvocationKillSwitchOpen);
    assert(!acceptedUpdate.executorInvocationKillSwitchPassed);
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
    assert(!acceptedDelete.realExecutionPolicyAllowed);
    assert(!acceptedDelete.executorOptInProvided);
    assert(!acceptedDelete.executorInjected);
    assert(!acceptedDelete.executorInvocationGuardPassed);
    assert(!acceptedDelete.executorResultMapped);
    assert(!acceptedDelete.executorResultSuccessful);
    assert(!acceptedDelete.executorInvocationAttempted);
    assert(!acceptedDelete.executorInvocationKillSwitchOpen);
    assert(!acceptedDelete.executorInvocationKillSwitchPassed);
    assert(acceptedDelete.dispatchStage == "command-request-mapped");
    assert(acceptedDelete.requiresBackendReadback);
    assert(acceptedDelete.backendId == "archive-vdr");
    assert(acceptedDelete.backendNativeId == "searchtimer-99");
    assert(acceptedDelete.operation == SearchTimerWorkflowOperation::Delete);
    assert(acceptedDelete.primaryStep == SearchTimerWorkflowExecutionStep::Delete);
    assert(acceptedDelete.followUpStep == SearchTimerWorkflowExecutionStep::Readback);
    assert(acceptedDelete.message == "delete command request accepted by dispatch skeleton");

    SuccessfulSearchTimerCommandExecutor successfulExecutor;

    const auto controlledReadbackCreatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Terra X Execute Verified",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    StaticSearchTimerDataSource createReadbackDataSource({
        makeTimer(
            "home-vdr",
            "created-native-1",
            "Terra X Execute Verified",
            "Terra X",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowExecutionResult controlledCreateWithReadback =
        dispatchService.dispatchPlan(
            controlledReadbackCreatePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocationAndReadbackDataSource(
                true,
                &successfulExecutor,
                &createReadbackDataSource));

    assert(controlledCreateWithReadback.success);
    assert(controlledCreateWithReadback.executed);
    assert(!controlledCreateWithReadback.blocked);
    assert(!controlledCreateWithReadback.dryRunOnly);
    assert(controlledCreateWithReadback.requiresBackendReadback);
    assert(controlledCreateWithReadback.backendReadbackVerificationAttached);
    assert(controlledCreateWithReadback.backendReadbackVerified());
    assert(controlledCreateWithReadback.backendReadbackVerification.passed());
    assert(controlledCreateWithReadback.backendNativeId == "created-native-1");
    assert(controlledCreateWithReadback.dispatchStage == "executor-result-mapped");
    assert(successfulExecutor.createCalls() == 1);

    SuccessfulSearchTimerCommandExecutor updateExecutor;

    const auto controlledReadbackUpdatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::update(
                "remote-vdr",
                "searchtimer-42",
                "Terra X Aktualisiert",
                "Terra X neu")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    StaticSearchTimerDataSource updateReadbackDataSource({
        makeTimer(
            "remote-vdr",
            "searchtimer-42",
            "Terra X Aktualisiert",
            "Terra X neu",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowExecutionResult controlledUpdateWithReadback =
        dispatchService.dispatchPlan(
            controlledReadbackUpdatePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocationAndReadbackDataSource(
                true,
                &updateExecutor,
                &updateReadbackDataSource));

    assert(controlledUpdateWithReadback.success);
    assert(controlledUpdateWithReadback.executed);
    assert(controlledUpdateWithReadback.requiresBackendReadback);
    assert(controlledUpdateWithReadback.backendReadbackVerificationAttached);
    assert(controlledUpdateWithReadback.backendReadbackVerified());
    assert(controlledUpdateWithReadback.backendReadbackVerification.passed());
    assert(controlledUpdateWithReadback.backendNativeId == "searchtimer-42");
    assert(controlledUpdateWithReadback.dispatchStage == "executor-result-mapped");
    assert(updateExecutor.updateCalls() == 1);

    SuccessfulSearchTimerCommandExecutor deleteExecutor;

    const auto controlledReadbackDeletePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::remove(
                "archive-vdr",
                "searchtimer-99")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    StaticSearchTimerDataSource deleteReadbackDataSource({
        makeTimer(
            "archive-vdr",
            "other-native",
            "Other SearchTimer",
            "Other",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowExecutionResult controlledDeleteWithReadback =
        dispatchService.dispatchPlan(
            controlledReadbackDeletePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocationAndReadbackDataSource(
                true,
                &deleteExecutor,
                &deleteReadbackDataSource));

    assert(controlledDeleteWithReadback.success);
    assert(controlledDeleteWithReadback.executed);
    assert(controlledDeleteWithReadback.requiresBackendReadback);
    assert(controlledDeleteWithReadback.backendReadbackVerificationAttached);
    assert(controlledDeleteWithReadback.backendReadbackVerified());
    assert(controlledDeleteWithReadback.backendReadbackVerification.passed());
    assert(controlledDeleteWithReadback.backendNativeId == "searchtimer-99");
    assert(controlledDeleteWithReadback.dispatchStage == "executor-result-mapped");
    assert(deleteExecutor.deleteCalls() == 1);

    SuccessfulSearchTimerCommandExecutor failedReadbackExecutor;

    const auto failedReadbackCreatePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Missing Readback",
                "Missing")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult createWithoutReadbackDataSource =
        dispatchService.dispatchPlan(
            failedReadbackCreatePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &failedReadbackExecutor));

    assert(!createWithoutReadbackDataSource.success);
    assert(createWithoutReadbackDataSource.executed);
    assert(createWithoutReadbackDataSource.backendReadbackVerificationAttached);
    assert(!createWithoutReadbackDataSource.backendReadbackVerified());
    assert(createWithoutReadbackDataSource.hasErrors());
    assert(createWithoutReadbackDataSource.dispatchStage == "backend-readback-verification-failed");

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
    assert(!listResult.realExecutionPolicyAllowed);
    assert(!listResult.executorOptInProvided);
    assert(!listResult.executorInjected);
    assert(!listResult.executorInvocationGuardPassed);
    assert(!listResult.executorResultMapped);
    assert(!listResult.executorResultSuccessful);
    assert(!listResult.executorInvocationAttempted);
    assert(!listResult.executorInvocationKillSwitchOpen);
    assert(!listResult.executorInvocationKillSwitchPassed);
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
    assert(!invalidResult.realExecutionPolicyAllowed);
    assert(!invalidResult.executorOptInProvided);
    assert(!invalidResult.executorInjected);
    assert(!invalidResult.executorInvocationGuardPassed);
    assert(!invalidResult.executorResultMapped);
    assert(!invalidResult.executorResultSuccessful);
    assert(!invalidResult.executorInvocationAttempted);
    assert(!invalidResult.executorInvocationKillSwitchOpen);
    assert(!invalidResult.executorInvocationKillSwitchPassed);
    assert(invalidResult.dispatchStage == "validation-blocked");
    assert(invalidResult.operation == SearchTimerWorkflowOperation::Update);
    assert(invalidResult.primaryStep == SearchTimerWorkflowExecutionStep::None);
    assert(invalidResult.hasErrors());

    std::cout
        << "test_search_timer_workflow_command_dispatch_service passed"
        << std::endl;

    return 0;
}
