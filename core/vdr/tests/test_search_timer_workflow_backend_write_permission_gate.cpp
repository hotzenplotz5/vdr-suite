#include "SearchTimerWorkflowBackendWritePermissionGate.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class BackendWritePermissionGateExecutor : public ISearchTimerCommandExecutor
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest&) override
    {
        return SearchTimerCreateResult::failed(
            "unexpected create call",
            {"unexpected create call"});
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest&) override
    {
        return SearchTimerUpdateResult::failed(
            "unexpected update call",
            {"unexpected update call"});
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest&) override
    {
        return SearchTimerDeleteResult::failed(
            "unexpected delete call",
            {"unexpected delete call"});
    }
};

int main()
{
    SearchTimerWorkflowPlanningService planningService;
    SearchTimerWorkflowBackendWritePermissionGate permissionGate;
    BackendWritePermissionGateExecutor executor;

    const SearchTimerWorkflowExecutionPlan writePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Permission Gate Write",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const auto missing =
        permissionGate.evaluate(
            writePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlist(
                true,
                &executor,
                std::vector<std::string>{"home-vdr"}));

    assert(!missing.permitted);
    assert(!missing.configured);
    assert(missing.backendId == "home-vdr");
    assert(missing.dispatchStage == "backend-write-permission-required");
    assert(!missing.errors.empty());

    const auto denied =
        permissionGate.evaluate(
            writePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlistAndPermission(
                true,
                &executor,
                std::vector<std::string>{"home-vdr"},
                std::vector<std::string>{"other-vdr"}));

    assert(!denied.permitted);
    assert(denied.configured);
    assert(denied.backendId == "home-vdr");
    assert(denied.dispatchStage == "backend-write-permission-denied");
    assert(!denied.errors.empty());

    const auto permitted =
        permissionGate.evaluate(
            writePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlistAndPermission(
                true,
                &executor,
                std::vector<std::string>{"home-vdr"},
                std::vector<std::string>{"home-vdr", "other-vdr"}));

    assert(permitted.permitted);
    assert(permitted.configured);
    assert(permitted.backendId == "home-vdr");
    assert(permitted.dispatchStage == "backend-write-permission-permitted");
    assert(permitted.errors.empty());

    const SearchTimerWorkflowExecutionPlan readPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::list(
                "home-vdr"));

    const auto readOnly =
        permissionGate.evaluate(
            readPlan,
            SearchTimerWorkflowCommandDispatchOptions::defaults());

    assert(readOnly.permitted);
    assert(readOnly.dispatchStage == "backend-write-permission-not-required");

    std::cout
        << "test_search_timer_workflow_backend_write_permission_gate passed"
        << std::endl;

    return 0;
}
