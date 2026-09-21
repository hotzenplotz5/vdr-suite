#include "SearchTimerAutomationDaemonSchedulingPlan.h"
#include "SearchTimerAutomationDryRunResult.h"
#include "SearchTimerAutomationEvaluationPlan.h"
#include "SearchTimerAutomationSafetyReview.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{

bool contains(
    const std::vector<std::string>& values,
    const std::string& expected)
{
    for (const auto& value : values)
    {
        if (value == expected)
        {
            return true;
        }
    }

    return false;
}

} // namespace

int main()
{
    SearchTimerAutomationEvaluationPlan plan =
        SearchTimerAutomationEvaluationPlan::createReadOnly("default");
    plan.setCandidateLimit(25);

    SearchTimerAutomationDryRunResult dryRunResult =
        SearchTimerAutomationDryRunResult::forPlan(plan);

    SearchTimerAutomationDaemonSchedulingPlan schedulingPlan =
        SearchTimerAutomationDaemonSchedulingPlan::previewOnly(
            "default",
            60);

    SearchTimerAutomationSafetyReview review;

    SearchTimerAutomationSafetyReviewResult result =
        review.review(
            dryRunResult,
            schedulingPlan);

    assert(result.safeForPreview);
    assert(!result.safeForSchedulingRuntime);
    assert(!result.safeForAutomaticExecution);
    assert(!result.safeForBackendMutation);
    assert(result.dryRunOnly);
    assert(result.explicitExecutionHandoffRequired);
    assert(result.reviewStage == "automation-preview-only-safe");
    assert(result.message == "SearchTimer automation is safe for read-only preview only");

    assert(contains(
        result.satisfiedConditions,
        "dry-run result is valid"));
    assert(contains(
        result.satisfiedConditions,
        "daemon scheduling plan is valid"));
    assert(contains(
        result.satisfiedConditions,
        "dry-run only is enforced"));
    assert(contains(
        result.satisfiedConditions,
        "mutation is disabled"));
    assert(contains(
        result.satisfiedConditions,
        "timer creation is disabled"));
    assert(contains(
        result.satisfiedConditions,
        "backend writes are disabled"));
    assert(contains(
        result.satisfiedConditions,
        "automatic execution is disabled"));
    assert(contains(
        result.satisfiedConditions,
        "scheduler runtime is disabled"));
    assert(contains(
        result.satisfiedConditions,
        "explicit execution handoff is required"));
    assert(contains(
        result.satisfiedConditions,
        "operator review before execution is required"));
    assert(contains(
        result.satisfiedConditions,
        "operator review for duplicates is required"));

    assert(result.hasBlockers());
    assert(contains(
        result.blockers,
        "daemon scheduling runtime is intentionally not enabled"));
    assert(contains(
        result.blockers,
        "automatic execution is intentionally not enabled"));
    assert(contains(
        result.blockers,
        "backend mutation is intentionally not enabled"));

    SearchTimerAutomationEvaluationPlan invalidPlan =
        SearchTimerAutomationEvaluationPlan::createReadOnly("");

    SearchTimerAutomationDryRunResult invalidDryRunResult =
        SearchTimerAutomationDryRunResult::forPlan(invalidPlan);

    SearchTimerAutomationDaemonSchedulingPlan invalidSchedulingPlan =
        SearchTimerAutomationDaemonSchedulingPlan::disabled("");

    SearchTimerAutomationSafetyReviewResult invalidResult =
        review.review(
            invalidDryRunResult,
            invalidSchedulingPlan);

    assert(!invalidResult.safeForPreview);
    assert(!invalidResult.safeForSchedulingRuntime);
    assert(!invalidResult.safeForAutomaticExecution);
    assert(!invalidResult.safeForBackendMutation);
    assert(invalidResult.reviewStage == "automation-safety-review-not-ready");
    assert(contains(
        invalidResult.blockers,
        "dry-run result is invalid"));
    assert(contains(
        invalidResult.blockers,
        "daemon scheduling plan is invalid"));
    assert(contains(
        invalidResult.blockers,
        "daemon scheduling runtime is intentionally not enabled"));
    assert(contains(
        invalidResult.blockers,
        "automatic execution is intentionally not enabled"));
    assert(contains(
        invalidResult.blockers,
        "backend mutation is intentionally not enabled"));

    std::cout
        << "test_search_timer_automation_safety_review passed"
        << std::endl;

    return 0;
}
