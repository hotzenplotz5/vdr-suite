#pragma once

#include "RecordingAction.h"

#include <string>
#include <vector>

struct RecordingActionPermissionEvaluationResult
{
    RecordingActionType type = RecordingActionType::Unknown;

    bool allowed = false;

    std::vector<std::string> grantedPermissions;
    std::vector<std::string> missingPermissions;
};
