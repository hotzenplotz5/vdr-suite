#pragma once

#include "SearchTimerWorkflowCommandDispatchOptions.h"
#include "SearchTimerWorkflowExecutionPlan.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowRealExecutionReadinessReviewResult
{
    bool readyForRealBackendExecution = false;
    bool planExecutable = false;
    bool writeOperation = false;
    bool executeModeRequested = false;
    bool explicitOperatorConfirmationProvided = false;
    bool executorOptInProvided = false;
    bool executorInjected = false;
    bool controlledTestInvocationOnly = false;
    bool productionRealExecutionEnabled = false;
    bool backendWriteAllowlistConfigured = false;
    bool backendWriteAllowed = false;
    bool backendWritePermissionConfigured = false;
    bool backendWritePermitted = false;
    bool productionRealExecutionPolicyAvailable = false;
    std::string readinessStage = "real-backend-execution-not-ready";
    std::string message = "real backend execution is not ready";
    std::vector<std::string> satisfiedConditions;
    std::vector<std::string> blockers;

    bool hasBlockers() const
    {
        return !blockers.empty();
    }
};

class SearchTimerWorkflowRealExecutionReadinessReview
{
public:
    SearchTimerWorkflowRealExecutionReadinessReviewResult review(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerWorkflowCommandDispatchOptions& options) const;
};
