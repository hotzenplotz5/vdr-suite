#include "SearchTimerWorkflowBackendWriteAllowlist.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class BackendWriteAllowlistExecutor : public ISearchTimerCommandExecutor
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
    SearchTimerWorkflowBackendWriteAllowlist allowlist;
    BackendWriteAllowlistExecutor executor;

    const SearchTimerWorkflowExecutionPlan writePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Allowlist Write",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const auto missing =
        allowlist.evaluate(
            writePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabled(
                true,
                &executor));

    assert(!missing.allowed);
    assert(!missing.configured);
    assert(missing.backendId == "home-vdr");
    assert(missing.dispatchStage == "backend-write-allowlist-required");
    assert(!missing.errors.empty());

    const auto denied =
        allowlist.evaluate(
            writePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlist(
                true,
                &executor,
                std::vector<std::string>{"other-vdr"}));

    assert(!denied.allowed);
    assert(denied.configured);
    assert(denied.backendId == "home-vdr");
    assert(denied.dispatchStage == "backend-write-backend-not-allowed");
    assert(!denied.errors.empty());

    const auto allowed =
        allowlist.evaluate(
            writePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlist(
                true,
                &executor,
                std::vector<std::string>{"home-vdr", "other-vdr"}));

    assert(allowed.allowed);
    assert(allowed.configured);
    assert(allowed.backendId == "home-vdr");
    assert(allowed.dispatchStage == "backend-write-allowlist-allowed");
    assert(allowed.errors.empty());

    const SearchTimerWorkflowExecutionPlan readPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::list(
                "home-vdr"));

    const auto readOnly =
        allowlist.evaluate(
            readPlan,
            SearchTimerWorkflowCommandDispatchOptions::defaults());

    assert(readOnly.allowed);
    assert(readOnly.dispatchStage == "backend-write-allowlist-not-required");

    std::cout
        << "test_search_timer_workflow_backend_write_allowlist passed"
        << std::endl;

    return 0;
}
