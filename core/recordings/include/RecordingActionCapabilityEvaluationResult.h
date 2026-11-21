#pragma once

#include "RecordingAction.h"

#include <string>
#include <vector>

struct RecordingActionCapabilityEvaluationResult
{
    RecordingActionType type = RecordingActionType::Unknown;

    bool allowed = false;

    std::vector<std::string> availableCapabilities;
    std::vector<std::string> missingCapabilities;
};
