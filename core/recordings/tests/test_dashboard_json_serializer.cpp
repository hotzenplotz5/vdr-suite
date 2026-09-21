#include "DashboardJsonSerializer.h"

#include <cassert>
#include <iostream>

int main()
{
    DashboardOverview overview;

    overview.jobs.totalJobs = 12;
    overview.jobs.pendingJobs = 4;
    overview.jobs.runningJobs = 1;
    overview.jobs.doneJobs = 6;
    overview.jobs.failedJobs = 1;

    overview.jobs.latestJobId = 99;
    overview.jobs.latestJobType = "SHRINK";
    overview.jobs.latestJobStatus = "DONE";

    overview.recordings.recordingsTotal = 20;
    overview.recordings.recordingsWithMetadata = 18;
    overview.recordings.recordingsWithoutMetadata = 2;

    overview.recordings.latestRecordingId = 5;
    overview.recordings.latestRecordingTitle = "Tatort";

    DashboardJsonSerializer serializer;

    std::string json =
        serializer.serialize(overview);

    assert(json.find("\"jobs\"") != std::string::npos);
    assert(json.find("\"recordings\"") != std::string::npos);

    assert(json.find("\"totalJobs\":12")
           != std::string::npos);

    assert(json.find("\"recordingsTotal\":20")
           != std::string::npos);

    std::cout
        << json
        << std::endl;

    std::cout
        << "test_dashboard_json_serializer passed"
        << std::endl;

    return 0;
}
