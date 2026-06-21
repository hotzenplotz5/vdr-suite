#pragma once

#include <string>

struct SearchTimerUpdateRequest
{
    std::string backendId = "default";
    std::string backendNativeId;
    std::string name;
    std::string query;
    bool active = true;

    bool hasBackendId() const
    {
        return !backendId.empty();
    }

    bool hasBackendNativeId() const
    {
        return !backendNativeId.empty();
    }

    bool hasName() const
    {
        return !name.empty();
    }

    bool hasQuery() const
    {
        return !query.empty();
    }

    bool isActive() const
    {
        return active;
    }
};
