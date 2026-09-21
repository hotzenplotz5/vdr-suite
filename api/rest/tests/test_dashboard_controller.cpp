#include "DashboardController.h"

#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"
#include "Database.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "MetadataRepository.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        std::cerr << "database open failed" << std::endl;
        return 1;
    }

    JobRepository jobRepository(db);
    RecordingRepository recordingRepository(db);
    MetadataRepository metadataRepository(db);

    JobDashboardService jobDashboardService(jobRepository);

    RecordingDashboardService recordingDashboardService(
        recordingRepository,
        metadataRepository);

    DashboardFacade dashboardFacade(
        jobDashboardService,
        recordingDashboardService);

    DashboardJsonSerializer jsonSerializer;

    DashboardController controller(
        dashboardFacade,
        jsonSerializer);

    ApiResponse response =
        controller.getDashboard();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");

    assert(response.body.find("\"jobs\"") != std::string::npos);
    assert(response.body.find("\"recordings\"") != std::string::npos);
    assert(response.body.find("\"totalJobs\"") != std::string::npos);
    assert(response.body.find("\"recordingsTotal\"") != std::string::npos);

    db.close();

    std::cout
        << response.body
        << std::endl;

    std::cout
        << "test_dashboard_controller passed"
        << std::endl;

    return 0;
}
