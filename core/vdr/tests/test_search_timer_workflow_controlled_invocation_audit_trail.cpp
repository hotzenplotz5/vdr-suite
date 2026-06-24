#include "SearchTimerWorkflowCommandDispatchService.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class AuditTrailSearchTimerCommandExecutor : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++createCalls_;

        const SearchTimer created =
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    "audit-vdr",
                    "audit-created-1"),
                request.name,
                request.query,
                SearchTimerState::Active);

        return SearchTimerCreateResult::ok(
            created,
            "audit test executor create result mapped");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        ++updateCalls_;

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
            "audit test executor update result mapped");
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        ++deleteCalls_;

        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "audit test executor delete result mapped");
    }

    int totalCalls() const
    {
        return createCalls_ + updateCalls_ + deleteCalls_;
    }

private:
    int createCalls_ = 0;
    int updateCalls_ = 0;
    int deleteCalls_ = 0;
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
    AuditTrailSearchTimerCommandExecutor executor;

    const SearchTimerWorkflowExecutionPlan createPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "audit-vdr",
                "Terra X Audit",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult createResult =
        dispatchService.dispatchPlan(
            createPlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithControlledTestExecutorInvocation(
                true,
                &executor));

    assert(createResult.success);
    assert(createResult.executed);
    assert(createResult.executorInvocationAttempted);
    assert(createResult.executorResultMapped);
    assert(createResult.executorResultSuccessful);
    assert(!createResult.executorInvocationAuditTrail.empty());
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "executionMode=execute"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "commandRequestMapped=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "explicitOperatorConfirmation=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "executorOptInProvided=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "executorInjected=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "controlledTestExecutorInvocation=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "policyStage=real-execution-policy-controlled-test-allowed"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "policyAllowed=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "guardStage=guarded-executor-invocation-ready"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "guardPassed=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "killSwitchStage=executor-invocation-kill-switch-open"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "killSwitchOpen=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "killSwitchPassed=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "executorInvocationAttempted=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "executorResultMapped=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "executorResultSuccessful=true"));
    assert(hasAuditEntry(
        createResult.executorInvocationAuditTrail,
        "finalDispatchStage=executor-result-mapped"));
    assert(executor.totalCalls() == 1);

    const SearchTimerWorkflowExecutionPlan deniedPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "audit-vdr",
                "Terra X Denied",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const SearchTimerWorkflowExecutionResult deniedResult =
        dispatchService.dispatchPlan(
            deniedPlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithExecutorOptInAndExecutor(
                true,
                &executor));

    assert(!deniedResult.success);
    assert(!deniedResult.executed);
    assert(deniedResult.blocked);
    assert(!deniedResult.executorInvocationAttempted);
    assert(!deniedResult.executorResultMapped);
    assert(!deniedResult.executorInvocationAuditTrail.empty());
    assert(hasAuditEntry(
        deniedResult.executorInvocationAuditTrail,
        "controlledTestExecutorInvocation=false"));
    assert(hasAuditEntry(
        deniedResult.executorInvocationAuditTrail,
        "policyStage=real-execution-enable-switch-required"));
    assert(hasAuditEntry(
        deniedResult.executorInvocationAuditTrail,
        "policyAllowed=false"));
    assert(hasAuditEntry(
        deniedResult.executorInvocationAuditTrail,
        "killSwitchOpen=false"));
    assert(hasAuditEntry(
        deniedResult.executorInvocationAuditTrail,
        "executorInvocationAttempted=false"));
    assert(hasAuditEntry(
        deniedResult.executorInvocationAuditTrail,
        "executorResultMapped=false"));
    assert(executor.totalCalls() == 1);

    std::cout
        << "test_search_timer_workflow_controlled_invocation_audit_trail passed"
        << std::endl;

    return 0;
}
