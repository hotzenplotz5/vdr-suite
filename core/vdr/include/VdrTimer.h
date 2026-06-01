#pragma once

#include <string>

struct VdrTimer
{
    std::string id;
    std::string channelId;
    std::string eventId;

    std::string title;
    std::string subtitle;

    std::string startTime;
    std::string endTime;

    int priority;
    int lifetime;

    bool enabled;
    bool recording;
};
