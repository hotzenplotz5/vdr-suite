#pragma once

#include "SearchTimerWorkflowCommandDispatchOptions.h"
#include "SearchTimerWorkflowExecutionPlan.h"
#include "SearchTimerWorkflowRealExecutionPolicy.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowGuardedExecutorInvocationDecision
{
    bool guardPassed = false;
    bool invocationAttempted = false;
    std::string dispatchStage = "executor-invocation-guard-blocked";
    std::string message = "guarded executor invocation is blocked";
    std::vector<std::string> errors = {
        "guarded executor invocation is blocked"};
};

class SearchTimerWorkflowGuardedExecutorInvocation
{
public:
    SearchTimerWorkflowGuardedExecutorInvocationDecision evaluate(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerWorkflowCommandDispatchOptions& options,
        const SearchTimerWorkflowRealExecutionPolicyDecision& policyDecision,
        bool commandRequestMapped) const;
};
