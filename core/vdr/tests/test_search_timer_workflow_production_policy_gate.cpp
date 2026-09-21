#include "SearchTimerWorkflowProductionPolicyGate.h"

#include "SearchTimerWorkflowPlanningService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class ProductionPolicyGateExecutor : public ISearchTimerCommandExecutor
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
    SearchTimerWorkflowProductionPolicyGate gate;
    ProductionPolicyGateExecutor executor;

    const SearchTimerWorkflowExecutionPlan writePlan =
        planningService.plan(
            SearchTimerWorkflowRequest::create(
                "home-vdr",
                "Production Policy Gate",
                "Terra X")
                .withExecutionMode(SearchTimerWorkflowExecutionMode::Execute));

    const auto missing =
        gate.evaluate(
            writePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlistAndPermission(
                true,
                &executor,
                std::vector<std::string>{"home-vdr"},
                std::vector<std::string>{"home-vdr"}));

    assert(!missing.allowed);
    assert(!missing.configured);
    assert(missing.dispatchStage == "production-policy-gate-required");
    assert(missing.message == "production policy gate is required");
    assert(!missing.errors.empty());

    const auto closed =
        gate.evaluate(
            writePlan,
            SearchTimerWorkflowCommandDispatchOptions::confirmedWithProductionRealExecutionEnabledAndBackendWriteAllowlistAndPermissionAndProductionPolicyGate(
                true,
                &executor,
                std::vector<std::string>{"home-vdr"},
                std::vector<std::string>{"home-vdr"}));

    assert(!closed.allowed);
    assert(closed.configured);
    assert(closed.dispatchStage == "production-policy-gate-closed");
    assert(closed.message == "production policy gate is closed for real backend mutation");
    assert(!closed.errors.empty());

    const SearchTimerWorkflowExecutionPlan readPlan =
        planningService.plan(
            SearchTimerWorkflowRequest::list(
                "home-vdr"));

    const auto readOnly =
        gate.evaluate(
            readPlan,
            SearchTimerWorkflowCommandDispatchOptions::defaults());

    assert(readOnly.allowed);
    assert(readOnly.configured);
    assert(readOnly.dispatchStage == "production-policy-gate-not-required");

    std::cout
        << "test_search_timer_workflow_production_policy_gate passed"
        << std::endl;

    return 0;
}
