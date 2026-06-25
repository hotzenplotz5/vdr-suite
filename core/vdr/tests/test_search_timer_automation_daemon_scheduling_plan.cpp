#include "SearchTimerAutomationDaemonSchedulingPlan.h"

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
    SearchTimerAutomationDaemonSchedulingPlan disabledPlan =
        SearchTimerAutomationDaemonSchedulingPlan::disabled("default");

    assert(disabledPlan.backendId() == "default");
    assert(!disabledPlan.enabled());
    assert(disabledPlan.previewOnly());
    assert(!disabledPlan.schedulerRuntimeAllowed());
    assert(!disabledPlan.automaticExecutionAllowed());
    assert(!disabledPlan.backendWriteAllowed());
    assert(!disabledPlan.timerCreationAllowed());
    assert(!disabledPlan.mutationAllowed());
    assert(disabledPlan.dryRunOnly());
    assert(disabledPlan.requiresExplicitExecutionHandoff());
    assert(disabledPlan.requireOperatorReviewBeforeExecution());
    assert(disabledPlan.requireOperatorReviewForDuplicates());
    assert(disabledPlan.requireFreshSnapshot());
    assert(disabledPlan.intervalMinutes() == 60);
    assert(disabledPlan.maximumCandidatesPerRun() == 50);
    assert(disabledPlan.maximumSnapshotAgeSeconds() == 300);
    assert(disabledPlan.isValid());
    assert(disabledPlan.validationErrors().empty());
    assert(disabledPlan.hasAuditTrail());
    assert(contains(
        disabledPlan.auditTrail(),
        "daemon scheduling plan created disabled"));

    const std::vector<std::string> invariants =
        disabledPlan.safetyInvariants();

    assert(contains(invariants, "scheduler runtime is disabled"));
    assert(contains(invariants, "automatic execution is disabled"));
    assert(contains(invariants, "backend writes are disabled"));
    assert(contains(invariants, "timer creation is disabled"));
    assert(contains(invariants, "mutation is disabled"));
    assert(contains(invariants, "dry-run only is enforced"));
    assert(contains(invariants, "explicit execution handoff is required"));

    SearchTimerAutomationDaemonSchedulingPlan previewPlan =
        SearchTimerAutomationDaemonSchedulingPlan::previewOnly(
            "livingroom",
            15);

    previewPlan.setMaximumCandidatesPerRun(25);
    previewPlan.setRequireFreshSnapshot(false);
    previewPlan.setMaximumSnapshotAgeSeconds(120);
    previewPlan.setRequireOperatorReviewForDuplicates(true);
    previewPlan.addSafetyReason("preview endpoint only");
    previewPlan.addSafetyReason("");

    assert(previewPlan.backendId() == "livingroom");
    assert(previewPlan.enabled());
    assert(previewPlan.previewOnly());
    assert(previewPlan.intervalMinutes() == 15);
    assert(previewPlan.maximumCandidatesPerRun() == 25);
    assert(!previewPlan.requireFreshSnapshot());
    assert(previewPlan.maximumSnapshotAgeSeconds() == 120);
    assert(previewPlan.requireOperatorReviewForDuplicates());
    assert(previewPlan.hasSafetyReasons());
    assert(previewPlan.safetyReasons().size() == 1);
    assert(previewPlan.safetyReasons().front() == "preview endpoint only");
    assert(!previewPlan.schedulerRuntimeAllowed());
    assert(!previewPlan.automaticExecutionAllowed());
    assert(!previewPlan.backendWriteAllowed());
    assert(!previewPlan.timerCreationAllowed());
    assert(!previewPlan.mutationAllowed());

    previewPlan.setIntervalMinutes(0);
    assert(previewPlan.intervalMinutes() == 1);

    previewPlan.setMaximumCandidatesPerRun(0);
    assert(previewPlan.maximumCandidatesPerRun() == 1);

    previewPlan.setMaximumSnapshotAgeSeconds(-1);
    assert(previewPlan.maximumSnapshotAgeSeconds() == 0);

    SearchTimerAutomationDaemonSchedulingPlan invalidPlan =
        SearchTimerAutomationDaemonSchedulingPlan::disabled("");

    assert(!invalidPlan.isValid());

    const std::vector<std::string> errors =
        invalidPlan.validationErrors();

    assert(errors.size() == 1);
    assert(errors[0] == "backend id is required");
    assert(invalidPlan.dryRunOnly());
    assert(!invalidPlan.mutationAllowed());
    assert(!invalidPlan.schedulerRuntimeAllowed());
    assert(!invalidPlan.automaticExecutionAllowed());

    std::cout
        << "test_search_timer_automation_daemon_scheduling_plan passed"
        << std::endl;

    return 0;
}
