#pragma once

#include <string>

struct SearchTimerCreateRequest
{
    std::string backendId = "default";
    std::string name;
    std::string query;
    bool active = true;
    std::string directory;
    int priority = 0;
    int lifetime = 0;
    int marginStartMinutes = 0;
    int marginStopMinutes = 0;
    bool useVps = false;

    bool hasBackendId() const
    {
        return !backendId.empty();
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
