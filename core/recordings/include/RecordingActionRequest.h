#pragma once

#include "RecordingAction.h"

#include <map>
#include <string>

struct RecordingActionRequest
{
    std::string backendId;
    std::string recordingId;
    RecordingActionType type = RecordingActionType::Unknown;
    bool dryRun = true;
    std::map<std::string, std::string> parameters;
};
