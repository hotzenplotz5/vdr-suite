#pragma once

#include "RecordingAction.h"

#include <string>
#include <vector>

struct RecordingActionExecutionResult
{
    RecordingActionType type = RecordingActionType::Unknown;

    bool success = false;

    std::string backendId;
    std::string recordingId;
    std::string message;

    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};
