#pragma once

#include <string>

struct SearchTimerDeleteRequest
{
    std::string backendId = "default";
    std::string backendNativeId;

    bool hasBackendId() const
    {
        return !backendId.empty();
    }

    bool hasBackendNativeId() const
    {
        return !backendNativeId.empty();
    }
};
