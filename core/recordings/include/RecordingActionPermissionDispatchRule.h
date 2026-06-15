#pragma once

#include <string>

struct RecordingActionPermissionDispatchRule
{
    std::string action;

    std::string requiredPermission;

    bool permissionRequired = true;
};
