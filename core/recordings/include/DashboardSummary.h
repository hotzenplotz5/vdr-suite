#pragma once

#include <string>

struct DashboardSummary
{
    int totalJobs = 0;

    int pendingJobs = 0;
    int runningJobs = 0;
    int doneJobs = 0;
    int failedJobs = 0;

    int latestJobId = 0;

    std::string latestJobType;
    std::string latestJobStatus;
};
