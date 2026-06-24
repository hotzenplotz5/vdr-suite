#include "SearchTimerWorkflowProductionExecutorHardeningPlan.h"

namespace
{

SearchTimerWorkflowProductionExecutorHardeningRequirement requirement(
    const std::string& id,
    const std::string& title,
    bool mandatory,
    bool satisfied,
    const std::string& status,
    const std::string& detail)
{
    SearchTimerWorkflowProductionExecutorHardeningRequirement item;
    item.id = id;
    item.title = title;
    item.mandatory = mandatory;
    item.satisfied = satisfied;
    item.status = status;
    item.detail = detail;
    return item;
}

void addRequirement(
    SearchTimerWorkflowProductionExecutorHardeningPlanResult& result,
    const SearchTimerWorkflowProductionExecutorHardeningRequirement& item)
{
    result.requirements.push_back(item);

    if (item.mandatory && !item.satisfied)
    {
        result.blockers.push_back(item.id + ": " + item.detail);
    }
}

} // namespace

SearchTimerWorkflowProductionExecutorHardeningPlanResult
SearchTimerWorkflowProductionExecutorHardeningPlan::build() const
{
    SearchTimerWorkflowProductionExecutorHardeningPlanResult result;

    addRequirement(
        result,
        requirement(
            "dry-run-prepare-execute-separation",
            "Dry-run, prepare and execute modes are separated",
            true,
            true,
            "satisfied",
            "workflow execution modes are explicit and tested"));

    addRequirement(
        result,
        requirement(
            "executor-opt-in-boundary",
            "Executor opt-in boundary exists",
            true,
            true,
            "satisfied",
            "execute mode requires executor opt-in before command dispatch"));

    addRequirement(
        result,
        requirement(
            "executor-injection-boundary",
            "Executor injection boundary exists",
            true,
            true,
            "satisfied",
            "real execution requires an injected command executor"));

    addRequirement(
        result,
        requirement(
            "guard-kill-switch-chain",
            "Guard and kill-switch chain exists",
            true,
            true,
            "satisfied",
            "guarded invocation and kill-switch contracts are in place"));

    addRequirement(
        result,
        requirement(
            "controlled-test-path",
            "Controlled test executor path exists",
            true,
            true,
            "satisfied",
            "fake executor invocation can prove the internal success path"));

    addRequirement(
        result,
        requirement(
            "audit-trail",
            "Executor invocation audit trail exists",
            true,
            true,
            "satisfied",
            "policy, guard, kill-switch, invocation and result mapping are recorded"));

    addRequirement(
        result,
        requirement(
            "readiness-review",
            "Real execution readiness review exists",
            true,
            true,
            "satisfied",
            "production readiness can be reported without backend mutation"));

    addRequirement(
        result,
        requirement(
            "production-enable-switch",
            "Production execution enable switch",
            true,
            false,
            "missing",
            "no production enable switch exists yet"));

    addRequirement(
        result,
        requirement(
            "backend-write-allowlist",
            "Backend write allowlist",
            true,
            false,
            "missing",
            "no backend write allowlist exists yet"));

    addRequirement(
        result,
        requirement(
            "per-backend-permission",
            "Per-backend write permission",
            true,
            false,
            "missing",
            "no per-backend write permission gate exists yet"));

    addRequirement(
        result,
        requirement(
            "production-policy-gate",
            "Production real-execution policy gate",
            true,
            false,
            "missing",
            "production real-execution policy gate is not available"));

    addRequirement(
        result,
        requirement(
            "mandatory-readback-verification",
            "Mandatory backend readback verification",
            true,
            false,
            "missing",
            "real backend execution does not yet require post-write readback verification"));

    addRequirement(
        result,
        requirement(
            "failure-compensation-plan",
            "Failure compensation plan",
            true,
            false,
            "missing",
            "no production failure compensation or rollback policy exists yet"));

    addRequirement(
        result,
        requirement(
            "rest-production-boundary",
            "REST production execution boundary",
            true,
            false,
            "missing",
            "REST does not yet expose a production-safe real execution boundary"));

    result.readyForProductionExecution = result.blockers.empty();

    return result;
}
