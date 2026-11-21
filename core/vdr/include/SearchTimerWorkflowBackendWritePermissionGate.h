#pragma once

#include "SearchTimerWorkflowCommandDispatchOptions.h"
#include "SearchTimerWorkflowExecutionPlan.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowBackendWritePermissionGateDecision
{
    bool permitted = false;
    bool configured = false;
    std::string backendId;
    std::string dispatchStage = "backend-write-permission-required";
    std::string message = "backend write permission gate is required";
    std::vector<std::string> permittedBackendIds;
    std::vector<std::string> errors;
};

class SearchTimerWorkflowBackendWritePermissionGate
{
public:
    SearchTimerWorkflowBackendWritePermissionGateDecision evaluate(
        const SearchTimerWorkflowExecutionPlan& plan,
        const SearchTimerWorkflowCommandDispatchOptions& options) const;
};
