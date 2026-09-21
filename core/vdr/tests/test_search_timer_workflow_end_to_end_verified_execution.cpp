#include "SearchTimerWorkflowCommandDispatchService.h"

#include "ISearchTimerCommandExecutor.h"
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

namespace
{

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

class EndToEndSearchTimerCommandExecutor final : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++createCalls_;
        return SearchTimerCreateResult::ok(
            makeTimer(
                request.backendId,
                "created-e2e-1",
                request.name,
                request.query,
                request.active
                    ? SearchTimerState::Active
                    : SearchTimerState::Inactive),
            "e2e create ok");
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
            "e2e update ok");
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        ++deleteCalls_;
        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "e2e delete ok");
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

}

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowCommandDispatchService dispatchService;

    EndToEndSearchTimerCommandExecutor createExecutor;
    StaticSearchTimerDataSource createReadbackDataSource({
        makeTimer(
            "home-vdr",
            "created-e2e-1",
            "E2E Create",
            "E2E Query",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowExecutionResult createResult =
        dispatchService.dispatchPlan(
            planningService.plan(
                SearchTimerWorkflowRequest::create(
                    "home-vdr",
                    "E2E Create",
                    "E2E Query")
                    .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute)),
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocationAndReadbackDataSource(
                true,
                &createExecutor,
                &createReadbackDataSource));

    assert(createResult.success);
    assert(createResult.executed);
    assert(!createResult.blocked);
    assert(!createResult.dryRunOnly);
    assert(createResult.confirmationProvided);
    assert(createResult.requiresBackendReadback);
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
    assert(createResult.backendReadbackVerificationAttached);
    assert(createResult.backendReadbackVerified());
    assert(createResult.backendReadbackVerification.passed());
    assert(createResult.operation == SearchTimerWorkflowOperation::Create);
    assert(createResult.primaryStep == SearchTimerWorkflowExecutionStep::Create);
    assert(createResult.followUpStep == SearchTimerWorkflowExecutionStep::Readback);
    assert(createResult.backendId == "home-vdr");
    assert(createResult.backendNativeId == "created-e2e-1");
    assert(createResult.dispatchStage == "executor-result-mapped");
    assert(createResult.message == "e2e create ok");
    assert(createExecutor.createCalls() == 1);
    assert(createExecutor.updateCalls() == 0);
    assert(createExecutor.deleteCalls() == 0);
    assert(contains(createResult.executorInvocationAuditTrail, "backendReadbackVerificationAttached=true"));
    assert(contains(createResult.executorInvocationAuditTrail, "backendReadbackVerified=true"));

    EndToEndSearchTimerCommandExecutor updateExecutor;
    StaticSearchTimerDataSource updateReadbackDataSource({
        makeTimer(
            "remote-vdr",
            "searchtimer-42",
            "E2E Update",
            "E2E Query Updated",
            SearchTimerState::Inactive)
    });

    const SearchTimerWorkflowExecutionResult updateResult =
        dispatchService.dispatchPlan(
            planningService.plan(
                SearchTimerWorkflowRequest::update(
                    "remote-vdr",
                    "searchtimer-42",
                    "E2E Update",
                    "E2E Query Updated",
                    false)
                    .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute)),
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocationAndReadbackDataSource(
                true,
                &updateExecutor,
                &updateReadbackDataSource));

    assert(updateResult.success);
    assert(updateResult.executed);
    assert(updateResult.requiresBackendReadback);
    assert(updateResult.executorResultSuccessful);
    assert(updateResult.backendReadbackVerificationAttached);
    assert(updateResult.backendReadbackVerified());
    assert(updateResult.backendReadbackVerification.passed());
    assert(updateResult.operation == SearchTimerWorkflowOperation::Update);
    assert(updateResult.primaryStep == SearchTimerWorkflowExecutionStep::Update);
    assert(updateResult.followUpStep == SearchTimerWorkflowExecutionStep::Readback);
    assert(updateResult.backendId == "remote-vdr");
    assert(updateResult.backendNativeId == "searchtimer-42");
    assert(updateResult.dispatchStage == "executor-result-mapped");
    assert(updateExecutor.createCalls() == 0);
    assert(updateExecutor.updateCalls() == 1);
    assert(updateExecutor.deleteCalls() == 0);
    assert(contains(updateResult.executorInvocationAuditTrail, "backendReadbackVerificationAttached=true"));
    assert(contains(updateResult.executorInvocationAuditTrail, "backendReadbackVerified=true"));

    EndToEndSearchTimerCommandExecutor deleteExecutor;
    StaticSearchTimerDataSource deleteReadbackDataSource({
        makeTimer(
            "archive-vdr",
            "other-native",
            "Other Timer",
            "Other Query",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowExecutionResult deleteResult =
        dispatchService.dispatchPlan(
            planningService.plan(
                SearchTimerWorkflowRequest::remove(
                    "archive-vdr",
                    "searchtimer-99")
                    .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute)),
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocationAndReadbackDataSource(
                true,
                &deleteExecutor,
                &deleteReadbackDataSource));

    assert(deleteResult.success);
    assert(deleteResult.executed);
    assert(deleteResult.requiresBackendReadback);
    assert(deleteResult.executorResultSuccessful);
    assert(deleteResult.backendReadbackVerificationAttached);
    assert(deleteResult.backendReadbackVerified());
    assert(deleteResult.backendReadbackVerification.passed());
    assert(deleteResult.operation == SearchTimerWorkflowOperation::Delete);
    assert(deleteResult.primaryStep == SearchTimerWorkflowExecutionStep::Delete);
    assert(deleteResult.followUpStep == SearchTimerWorkflowExecutionStep::Readback);
    assert(deleteResult.backendId == "archive-vdr");
    assert(deleteResult.backendNativeId == "searchtimer-99");
    assert(deleteResult.dispatchStage == "executor-result-mapped");
    assert(deleteExecutor.createCalls() == 0);
    assert(deleteExecutor.updateCalls() == 0);
    assert(deleteExecutor.deleteCalls() == 1);
    assert(contains(deleteResult.executorInvocationAuditTrail, "backendReadbackVerificationAttached=true"));
    assert(contains(deleteResult.executorInvocationAuditTrail, "backendReadbackVerified=true"));

    EndToEndSearchTimerCommandExecutor missingReadbackExecutor;
    StaticSearchTimerDataSource emptyReadbackDataSource({});

    const SearchTimerWorkflowExecutionResult failedReadbackResult =
        dispatchService.dispatchPlan(
            planningService.plan(
                SearchTimerWorkflowRequest::create(
                    "home-vdr",
                    "Missing Readback",
                    "Missing Query")
                    .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute)),
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocationAndReadbackDataSource(
                true,
                &missingReadbackExecutor,
                &emptyReadbackDataSource));

    assert(!failedReadbackResult.success);
    assert(failedReadbackResult.executed);
    assert(!failedReadbackResult.blocked);
    assert(!failedReadbackResult.dryRunOnly);
    assert(failedReadbackResult.executorResultSuccessful);
    assert(failedReadbackResult.backendReadbackVerificationAttached);
    assert(!failedReadbackResult.backendReadbackVerified());
    assert(!failedReadbackResult.backendReadbackVerification.passed());
    assert(failedReadbackResult.hasErrors());
    assert(failedReadbackResult.dispatchStage == "backend-readback-verification-failed");
    assert(missingReadbackExecutor.createCalls() == 1);
    assert(contains(failedReadbackResult.executorInvocationAuditTrail, "backendReadbackVerificationAttached=true"));
    assert(contains(failedReadbackResult.executorInvocationAuditTrail, "backendReadbackVerified=false"));

    std::cout
        << "test_search_timer_workflow_end_to_end_verified_execution passed"
        << std::endl;

    return 0;
}
