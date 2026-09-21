#include "Database.h"

#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "MetadataRepository.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        std::cerr << "failed to open database" << std::endl;
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

    DashboardJsonSerializer serializer;

    auto overview =
        facade.getOverview();

    std::cout
        << serializer.serialize(overview)
        << std::endl;

    db.close();

    return 0;
}
