#pragma once

#include <string>

struct RecordingActionCapabilityDispatchRule
{
    std::string action;

    std::string requiredCapability;

    bool capabilityRequired = true;
};
