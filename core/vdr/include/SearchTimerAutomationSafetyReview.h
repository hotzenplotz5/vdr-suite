#pragma once

#include "SearchTimerAutomationDaemonSchedulingPlan.h"
#include "SearchTimerAutomationDryRunResult.h"

#include <string>
#include <vector>

struct SearchTimerAutomationSafetyReviewResult
{
    bool safeForPreview = false;
    bool safeForSchedulingRuntime = false;
    bool safeForAutomaticExecution = false;
    bool safeForBackendMutation = false;
    bool dryRunOnly = true;
    bool explicitExecutionHandoffRequired = true;
    std::string reviewStage = "automation-safety-review-not-ready";
    std::string message = "SearchTimer automation is limited to read-only preview";
    std::vector<std::string> satisfiedConditions;
    std::vector<std::string> blockers;

    bool hasBlockers() const
    {
        return !blockers.empty();
    }
};

class SearchTimerAutomationSafetyReview
{
public:
    SearchTimerAutomationSafetyReviewResult review(
        const SearchTimerAutomationDryRunResult& dryRunResult,
        const SearchTimerAutomationDaemonSchedulingPlan& schedulingPlan) const;
};
