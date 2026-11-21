#pragma once

#include <string>

struct VdrTimer
{
    std::string id;
    std::string channelId;
    std::string channelName;
    std::string eventId;

    std::string title;
    std::string directory;
    std::string subtitle;
    std::string aux;

    std::string day;
    std::string weekdays = "-------";
    std::string startTime;
    std::string endTime;

    int flags = 0;
    int priority = 0;
    int lifetime = 0;

    bool enabled = false;
    bool vps = false;
    bool recording = false;
    bool pending = false;
};
