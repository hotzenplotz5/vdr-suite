#pragma once

#include <string>

struct SearchTimerUpdateRequest
{
    std::string backendId = "default";
    std::string backendNativeId;
    std::string name;
    std::string query;
    bool active = true;
    std::string directory;
    int priority = 0;
    int lifetime = 0;
    int marginStartMinutes = 0;
    int marginStopMinutes = 0;
    bool useVps = false;
    int useChannel = 0;
    std::string channels;
    std::string channelMin;
    std::string channelMax;

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
