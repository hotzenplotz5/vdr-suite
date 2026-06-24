#pragma once

#include "SearchTimerWorkflowCommandDispatchOptions.h"
#include "SearchTimerWorkflowExecutionPlan.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowRealExecutionPolicyDecision
{
    bool allowed = false;
    std::string dispatchStage = "real-execution-policy-denied";
    std::string message = "real execution policy denies backend command dispatch";
    std::vector<std::string> errors = {
        "real execution policy denies backend command dispatch"};
};

class SearchTimerWorkflowRealExecutionPolicy
{
public:
    SearchTimerWorkflowRealExecutionPolicyDecision evaluate(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerWorkflowCommandDispatchOptions& options) const;
};
