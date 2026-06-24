#include "SearchTimerWorkflowProductionExecutorHardeningPlanJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    SearchTimerWorkflowProductionExecutorHardeningPlan plan;
    SearchTimerWorkflowProductionExecutorHardeningPlanJsonSerializer serializer;

    const SearchTimerWorkflowProductionExecutorHardeningPlanResult result =
        plan.build();

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"readyForProductionExecution\":false") != std::string::npos);
    assert(json.find("\"requirements\":[") != std::string::npos);
    assert(json.find("\"id\":\"dry-run-prepare-execute-separation\"") != std::string::npos);
    assert(json.find("\"id\":\"production-enable-switch\"") != std::string::npos);
    assert(json.find("\"id\":\"backend-write-allowlist\"") != std::string::npos);
    assert(json.find("backend write allowlist contract exists") != std::string::npos);
    assert(json.find("\"id\":\"production-policy-gate\"") != std::string::npos);
    assert(json.find("\"status\":\"satisfied\"") != std::string::npos);
    assert(json.find("\"status\":\"missing\"") != std::string::npos);
    assert(json.find("production enable switch contract exists and is disabled by default") != std::string::npos);
    assert(json.find("production real-execution policy gate is not available") != std::string::npos);

    std::cout
        << "test_search_timer_workflow_production_executor_hardening_plan_json_serializer passed"
        << std::endl;

    return 0;
}
