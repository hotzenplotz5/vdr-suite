#pragma once

#include <string>

struct SearchTimerCreateRequest
{
    std::string backendId = "default";
    std::string name;
    std::string query;
    bool active = true;

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
