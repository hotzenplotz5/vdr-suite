#pragma once

#include "RecordingAction.h"

#include <map>
#include <string>
#include <vector>

struct RecordingActionJobPayload
{
    std::string backendId;
    std::string recordingId;
    RecordingActionType type = RecordingActionType::Unknown;
    std::string jobType;
    bool dryRun = true;
    std::map<std::string, std::string> parameters;
    std::vector<std::string> requiredCapabilities;
    std::vector<std::string> requiredPermissions;
    std::vector<std::string> warnings;
};
