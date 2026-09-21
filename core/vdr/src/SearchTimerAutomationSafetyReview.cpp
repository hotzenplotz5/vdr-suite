#include "SearchTimerAutomationSafetyReview.h"

namespace
{

void addCondition(
    std::vector<std::string>& entries,
    const std::string& entry)
{
    entries.push_back(entry);
}

void addBlocker(
    std::vector<std::string>& entries,
    const std::string& entry)
{
    entries.push_back(entry);
}

} // namespace

SearchTimerAutomationSafetyReviewResult SearchTimerAutomationSafetyReview::review(
    const SearchTimerAutomationDryRunResult& dryRunResult,
    const SearchTimerAutomationDaemonSchedulingPlan& schedulingPlan) const
{
    SearchTimerAutomationSafetyReviewResult result;

    result.dryRunOnly =
        dryRunResult.dryRunOnly()
        && schedulingPlan.dryRunOnly();

    result.explicitExecutionHandoffRequired =
        dryRunResult.requiresExplicitExecutionHandoff()
        && schedulingPlan.requiresExplicitExecutionHandoff();

    if (dryRunResult.isValid())
    {
        addCondition(
            result.satisfiedConditions,
            "dry-run result is valid");
    }
    else
    {
        addBlocker(
            result.blockers,
            "dry-run result is invalid");
    }

    if (schedulingPlan.isValid())
    {
        addCondition(
            result.satisfiedConditions,
            "daemon scheduling plan is valid");
    }
    else
    {
        addBlocker(
            result.blockers,
            "daemon scheduling plan is invalid");
    }

    if (result.dryRunOnly)
    {
        addCondition(
            result.satisfiedConditions,
            "dry-run only is enforced");
    }
    else
    {
        addBlocker(
            result.blockers,
            "dry-run only is not enforced");
    }

    if (!dryRunResult.mutationAllowed()
        && !schedulingPlan.mutationAllowed())
    {
        addCondition(
            result.satisfiedConditions,
            "mutation is disabled");
    }
    else
    {
        addBlocker(
            result.blockers,
            "mutation is allowed");
    }

    if (!dryRunResult.timerCreationAllowed()
        && !schedulingPlan.timerCreationAllowed())
    {
        addCondition(
            result.satisfiedConditions,
            "timer creation is disabled");
    }
    else
    {
        addBlocker(
            result.blockers,
            "timer creation is allowed");
    }

    if (!dryRunResult.backendWriteAllowed()
        && !schedulingPlan.backendWriteAllowed())
    {
        addCondition(
            result.satisfiedConditions,
            "backend writes are disabled");
    }
    else
    {
        addBlocker(
            result.blockers,
            "backend writes are allowed");
    }

    if (!dryRunResult.automaticExecutionAllowed()
        && !schedulingPlan.automaticExecutionAllowed())
    {
        addCondition(
            result.satisfiedConditions,
            "automatic execution is disabled");
    }
    else
    {
        addBlocker(
            result.blockers,
            "automatic execution is allowed");
    }

    if (!schedulingPlan.schedulerRuntimeAllowed())
    {
        addCondition(
            result.satisfiedConditions,
            "scheduler runtime is disabled");
    }
    else
    {
        addBlocker(
            result.blockers,
            "scheduler runtime is allowed");
    }

    if (result.explicitExecutionHandoffRequired)
    {
        addCondition(
            result.satisfiedConditions,
            "explicit execution handoff is required");
    }
    else
    {
        addBlocker(
            result.blockers,
            "explicit execution handoff is not required");
    }

    if (schedulingPlan.requireOperatorReviewBeforeExecution())
    {
        addCondition(
            result.satisfiedConditions,
            "operator review before execution is required");
    }
    else
    {
        addBlocker(
            result.blockers,
            "operator review before execution is not required");
    }

    if (schedulingPlan.requireOperatorReviewForDuplicates())
    {
        addCondition(
            result.satisfiedConditions,
            "operator review for duplicates is required");
    }
    else
    {
        addBlocker(
            result.blockers,
            "operator review for duplicates is not required");
    }

    result.safeForPreview =
        dryRunResult.isValid()
        && schedulingPlan.isValid()
        && result.dryRunOnly
        && !dryRunResult.mutationAllowed()
        && !dryRunResult.timerCreationAllowed()
        && !dryRunResult.backendWriteAllowed()
        && !dryRunResult.automaticExecutionAllowed()
        && !schedulingPlan.mutationAllowed()
        && !schedulingPlan.timerCreationAllowed()
        && !schedulingPlan.backendWriteAllowed()
        && !schedulingPlan.automaticExecutionAllowed()
        && !schedulingPlan.schedulerRuntimeAllowed()
        && result.explicitExecutionHandoffRequired;

    result.safeForSchedulingRuntime = false;
    result.safeForAutomaticExecution = false;
    result.safeForBackendMutation = false;

    addBlocker(
        result.blockers,
        "daemon scheduling runtime is intentionally not enabled");
    addBlocker(
        result.blockers,
        "automatic execution is intentionally not enabled");
    addBlocker(
        result.blockers,
        "backend mutation is intentionally not enabled");

    if (result.safeForPreview)
    {
        result.reviewStage = "automation-preview-only-safe";
        result.message =
            "SearchTimer automation is safe for read-only preview only";
    }
    else
    {
        result.reviewStage = "automation-safety-review-not-ready";
        result.message =
            "SearchTimer automation safety review found preview blockers";
    }

    return result;
}
