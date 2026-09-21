#pragma once

#include <string>

struct RecordingDashboardSummary
{
    int recordingsTotal = 0;

    int recordingsWithMetadata = 0;
    int recordingsWithoutMetadata = 0;

    int latestRecordingId = 0;

    std::string latestRecordingTitle;
};
