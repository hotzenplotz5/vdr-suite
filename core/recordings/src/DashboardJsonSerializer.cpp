#include "DashboardJsonSerializer.h"

#include <sstream>

std::string DashboardJsonSerializer::serialize(
    const DashboardOverview& overview) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"jobs\":{"
        << "\"totalJobs\":" << overview.jobs.totalJobs << ","
        << "\"pendingJobs\":" << overview.jobs.pendingJobs << ","
        << "\"runningJobs\":" << overview.jobs.runningJobs << ","
        << "\"doneJobs\":" << overview.jobs.doneJobs << ","
        << "\"failedJobs\":" << overview.jobs.failedJobs << ","
        << "\"latestJobId\":" << overview.jobs.latestJobId << ","
        << "\"latestJobType\":\"" << overview.jobs.latestJobType << "\","
        << "\"latestJobStatus\":\"" << overview.jobs.latestJobStatus << "\""
        << "},"

        << "\"recordings\":{"
        << "\"recordingsTotal\":"
        << overview.recordings.recordingsTotal << ","

        << "\"recordingsWithMetadata\":"
        << overview.recordings.recordingsWithMetadata << ","

        << "\"recordingsWithoutMetadata\":"
        << overview.recordings.recordingsWithoutMetadata << ","

        << "\"latestRecordingId\":"
        << overview.recordings.latestRecordingId << ","

        << "\"latestRecordingTitle\":\""
        << overview.recordings.latestRecordingTitle
        << "\""

        << "}"

        << "}";

    return json.str();
}
