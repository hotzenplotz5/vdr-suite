#include "ApiRouter.h"
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

    DashboardController dashboardController(
        dashboardFacade,
        jsonSerializer);

    ApiRouter router(dashboardController);

    ApiResponse dashboardResponse =
        router.handleGet("/api/dashboard");

    assert(dashboardResponse.statusCode == 200);
    assert(dashboardResponse.contentType == "application/json");
    assert(dashboardResponse.body.find("\"jobs\"") != std::string::npos);
    assert(dashboardResponse.body.find("\"recordings\"") != std::string::npos);

    ApiResponse missingResponse =
        router.handleGet("/api/unknown");

    assert(missingResponse.statusCode == 404);
    assert(missingResponse.contentType == "application/json");
    assert(missingResponse.body.find("\"error\"") != std::string::npos);

    db.close();

    std::cout
        << dashboardResponse.body
        << std::endl;

    std::cout
        << missingResponse.body
        << std::endl;

    std::cout
        << "test_api_router passed"
        << std::endl;

    return 0;
}
