#pragma once

#include "RecordingAction.h"

#include <string>
#include <vector>

struct RecordingActionExecutionReadinessResult
{
    RecordingActionType type = RecordingActionType::Unknown;

    bool ready = false;

    std::vector<std::string> readinessChecksPassed;
    std::vector<std::string> readinessChecksFailed;
};
