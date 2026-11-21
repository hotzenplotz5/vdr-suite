#pragma once

#include "RecordingAction.h"

#include <string>
#include <vector>

struct RecordingActionPlan
{
    std::string backendId;
    std::string recordingId;
    RecordingActionType type = RecordingActionType::Unknown;
    bool dryRun = true;
    bool executionAllowed = false;
    bool createJob = false;
    std::string plannedJobType;
    std::vector<std::string> requiredCapabilities;
    std::vector<std::string> requiredPermissions;
    std::vector<std::string> warnings;
};
