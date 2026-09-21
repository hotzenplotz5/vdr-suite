#pragma once

#include "SearchTimerWorkflowCommandDispatchOptions.h"
#include "SearchTimerWorkflowExecutionPlan.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowBackendWriteAllowlistDecision
{
    bool allowed = false;
    bool configured = false;
    std::string backendId;
    std::string dispatchStage = "backend-write-allowlist-required";
    std::string message = "backend write allowlist is required";
    std::vector<std::string> allowedBackendIds;
    std::vector<std::string> errors;
};

class SearchTimerWorkflowBackendWriteAllowlist
{
public:
    SearchTimerWorkflowBackendWriteAllowlistDecision evaluate(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerWorkflowCommandDispatchOptions& options) const;
};
