#pragma once

#include "SearchTimerWorkflowRequest.h"

#include <string>
#include <vector>

struct SearchTimerWorkflowValidationResult
{
    bool valid = false;
    bool readOnly = true;
    bool writeOperation = false;
    bool wantsReadbackAfterWrite = false;
    SearchTimerWorkflowOperation operation =
        SearchTimerWorkflowOperation::Unknown;
    std::string backendId;
    std::string backendNativeId;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};
