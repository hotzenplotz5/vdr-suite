#include "DashboardFacade.h"
#include "Database.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "MetadataRepository.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"

#include <cassert>
#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        return 1;
    }

    JobRepository jobRepository(db);
    RecordingRepository recordingRepository(db);
    MetadataRepository metadataRepository(db);

    JobDashboardService jobDashboardService(jobRepository);

    RecordingDashboardService recordingDashboardService(
        recordingRepository,
        metadataRepository);

    DashboardFacade facade(
        jobDashboardService,
        recordingDashboardService);

    DashboardOverview overview =
        facade.getOverview();

    assert(overview.jobs.totalJobs >= 2);
    assert(overview.jobs.pendingJobs >= 1);
    assert(overview.jobs.doneJobs >= 1);

    assert(overview.recordings.recordingsTotal == 2);
    assert(overview.recordings.recordingsWithMetadata == 1);
    assert(overview.recordings.recordingsWithoutMetadata == 1);
    assert(overview.recordings.latestRecordingId == 2);
    assert(overview.recordings.latestRecordingTitle == "Tagesschau");

    db.close();

    std::cout
        << "test_dashboard_facade passed"
        << std::endl;

    return 0;
}
