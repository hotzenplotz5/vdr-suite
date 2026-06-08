#pragma once

#include <string>
#include <vector>

struct VdrEvent
{
    std::string id;
    std::string channelId;

    std::string title;
    std::string subtitle;
    std::string description;

    std::string startTime;
    std::string endTime;

    int durationSeconds;

    std::vector<std::string> contentDescriptors;
    int parentalRating;
};
