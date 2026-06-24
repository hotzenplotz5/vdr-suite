#include "SearchTimerWorkflowProductionExecutorHardeningPlan.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

bool hasRequirement(
    const std::vector<SearchTimerWorkflowProductionExecutorHardeningRequirement>& requirements,
    const std::string& id,
    bool satisfied)
{
    for (const auto& item : requirements)
    {
        if (item.id == id && item.satisfied == satisfied)
        {
            return true;
        }
    }

    return false;
}

bool hasBlockerContaining(
    const std::vector<std::string>& blockers,
    const std::string& expected)
{
    for (const std::string& blocker : blockers)
    {
        if (blocker.find(expected) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

int main()
{
    SearchTimerWorkflowProductionExecutorHardeningPlan plan;
    const SearchTimerWorkflowProductionExecutorHardeningPlanResult result =
        plan.build();

    assert(!result.readyForProductionExecution);
    assert(result.hasBlockers());
    assert(!result.requirements.empty());

    assert(hasRequirement(
        result.requirements,
        "dry-run-prepare-execute-separation",
        true));
    assert(hasRequirement(
        result.requirements,
        "executor-opt-in-boundary",
        true));
    assert(hasRequirement(
        result.requirements,
        "executor-injection-boundary",
        true));
    assert(hasRequirement(
        result.requirements,
        "guard-kill-switch-chain",
        true));
    assert(hasRequirement(
        result.requirements,
        "controlled-test-path",
        true));
    assert(hasRequirement(
        result.requirements,
        "audit-trail",
        true));
    assert(hasRequirement(
        result.requirements,
        "readiness-review",
        true));

    assert(hasRequirement(
        result.requirements,
        "production-enable-switch",
        true));
    assert(hasRequirement(
        result.requirements,
        "backend-write-allowlist",
        true));
    assert(hasRequirement(
        result.requirements,
        "per-backend-permission",
        true));
    assert(hasRequirement(
        result.requirements,
        "production-policy-gate",
        false));
    assert(hasRequirement(
        result.requirements,
        "mandatory-readback-verification",
        false));
    assert(hasRequirement(
        result.requirements,
        "failure-compensation-plan",
        false));
    assert(hasRequirement(
        result.requirements,
        "rest-production-boundary",
        false));

    assert(hasBlockerContaining(
        result.blockers,
        "production real-execution policy gate is not available"));

    std::cout
        << "test_search_timer_workflow_production_executor_hardening_plan passed"
        << std::endl;

    return 0;
}
