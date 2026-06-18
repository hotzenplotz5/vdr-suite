#pragma once

#include <string>

struct VdrTimerOperationRequest
{
    std::string timerId;
    std::string backendId;

    std::string channelId;

    std::string title;
    std::string directory;

    std::string day;
    std::string weekdays = "-------";

    int start = 0;
    int stop = 0;

    int priority = 50;
    int lifetime = 99;

    bool active = true;
    bool vps = false;

    std::string aux;

    bool hasTimerId() const
    {
        return !timerId.empty();
    }

    bool hasBackendId() const
    {
        return !backendId.empty();
    }

    bool hasDirectory() const
    {
        return !directory.empty();
    }

    bool hasDay() const
    {
        return !day.empty();
    }

    bool isRepeating() const
    {
        return weekdays != "-------";
    }

    bool hasTimeWindow() const
    {
        return start > 0 && stop > 0;
    }

    bool isActive() const
    {
        return active;
    }

    bool usesVps() const
    {
        return vps;
    }
};
