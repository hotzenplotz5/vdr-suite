#include "Database.h"
#include "JobDashboardService.h"
#include "JobRepository.h"

#include <cassert>
#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        std::cerr << "database open failed" << std::endl;
        return 1;
    }

    JobRepository repository(db);
    JobDashboardService dashboard(repository);

    DashboardSummary before =
        dashboard.getSummary();

    Job job1;
    job1.recordingId = 1;
    job1.jobType = "IMPORT";
    job1.status = "PENDING";

    Job job2;
    job2.recordingId = 1;
    job2.jobType = "CUT";
    job2.status = "DONE";

    Job job3;
    job3.recordingId = 1;
    job3.jobType = "SHRINK";
    job3.status = "FAILED";

    assert(repository.insertJob(job1));
    assert(repository.insertJob(job2));
    assert(repository.insertJob(job3));

    DashboardSummary after =
        dashboard.getSummary();

    assert(after.totalJobs == before.totalJobs + 3);
    assert(after.pendingJobs == before.pendingJobs + 1);
    assert(after.runningJobs == before.runningJobs);
    assert(after.doneJobs == before.doneJobs + 1);
    assert(after.failedJobs == before.failedJobs + 1);

    assert(after.latestJobId > before.latestJobId);
    assert(after.latestJobType == "SHRINK");
    assert(after.latestJobStatus == "FAILED");

    db.close();

    std::cout << "test_job_dashboard_service passed" << std::endl;

    return 0;
}
