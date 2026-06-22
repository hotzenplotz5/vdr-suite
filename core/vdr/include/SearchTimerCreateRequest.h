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
    int useChannel = 0;
    std::string channels;
    std::string channelMin;
    std::string channelMax;
    bool useTime = false;
    int startTime = 0;
    int stopTime = 0;
    bool useDuration = false;
    int durationMinMinutes = 0;
    int durationMaxMinutes = 0;
    bool useDayOfWeek = false;
    int dayOfWeek = 0;
    bool avoidRepeats = false;
    int allowedRepeats = 0;
    int repeatsWithinDays = 0;
    bool compareTitle = false;
    bool compareSubtitle = false;
    bool compareSummary = false;
    bool compareCategories = false;
    bool compareTime = false;
    bool useSeriesRecording = false;
    int keepRecordings = 0;
    int deleteMode = 0;
    int searchTimerAction = 0;
    int blacklistMode = 0;
    std::string blacklistIds;
    int matchMode = 0;
    bool matchCase = false;
    int matchTolerance = 0;
    int summaryMatch = 0;

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
