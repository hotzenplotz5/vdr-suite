#pragma once

#include "SearchTimerWorkflowCommandDispatchOptions.h"
#include "SearchTimerWorkflowExecutionPlan.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowProductionPolicyGateDecision
{
    bool allowed = false;
    bool configured = false;
    std::string dispatchStage = "production-policy-gate-required";
    std::string message = "production policy gate is required";
    std::vector<std::string> errors;
};

class SearchTimerWorkflowProductionPolicyGate
{
public:
    SearchTimerWorkflowProductionPolicyGateDecision evaluate(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerWorkflowCommandDispatchOptions& options) const;
};
