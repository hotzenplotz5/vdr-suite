#include "JobDashboardService.h"
#include "JobRepository.h"

JobDashboardService::JobDashboardService(
    JobRepository& repository)
    : repository_(repository)
{
}

DashboardSummary JobDashboardService::getSummary()
{
    DashboardSummary summary;

    const auto jobs =
        repository_.getAllJobs();

    summary.totalJobs =
        static_cast<int>(jobs.size());

    for (const auto& job : jobs)
    {
        if (job.status == "PENDING")
        {
            ++summary.pendingJobs;
        }
        else if (job.status == "RUNNING")
        {
            ++summary.runningJobs;
        }
        else if (job.status == "DONE")
        {
            ++summary.doneJobs;
        }
        else if (job.status == "FAILED")
        {
            ++summary.failedJobs;
        }

        if (job.id > summary.latestJobId)
        {
            summary.latestJobId = job.id;
            summary.latestJobType = job.jobType;
            summary.latestJobStatus = job.status;
        }
    }

    return summary;
}
