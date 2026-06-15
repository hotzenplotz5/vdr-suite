#pragma once

#include "RecordingAction.h"

#include <string>
#include <vector>

struct RecordingActionCapabilityRequirements
{
    RecordingActionType type = RecordingActionType::Unknown;
    std::vector<std::string> requiredCapabilities;
    bool requiresWriteAccess = false;
    bool requiresDryRunSupport = false;
};
