#include "ApiRouter.h"
#include "DashboardController.h"
#include "JobsController.h"
#include "MetadataController.h"
#include "RecordingsController.h"

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

    JobsController jobsController(jobRepository);

    RecordingsController recordingsController(
        recordingRepository);

    MetadataController metadataController(
        metadataRepository);

    ApiRouter router(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController);

    ApiResponse dashboardResponse =
        router.handleGet("/api/dashboard");

    assert(dashboardResponse.statusCode == 200);
    assert(dashboardResponse.contentType == "application/json");
    assert(dashboardResponse.body.find("\"jobs\"") != std::string::npos);
    assert(dashboardResponse.body.find("\"recordings\"") != std::string::npos);

    ApiResponse jobsResponse =
        router.handleGet("/api/jobs");

    assert(jobsResponse.statusCode == 200);
    assert(jobsResponse.contentType == "application/json");
    assert(jobsResponse.body.find("\"jobs\"") != std::string::npos);
    assert(jobsResponse.body.find("\"jobType\"") != std::string::npos);
    assert(jobsResponse.body.find("\"status\"") != std::string::npos);

    ApiResponse recordingsResponse =
        router.handleGet("/api/recordings");

    assert(recordingsResponse.statusCode == 200);
    assert(recordingsResponse.contentType == "application/json");
    assert(recordingsResponse.body.find("\"recordings\"") != std::string::npos);
    assert(recordingsResponse.body.find("\"title\":\"Tatort\"") != std::string::npos);
    assert(recordingsResponse.body.find("\"recordingFormat\":\"TS\"") != std::string::npos);

    ApiResponse metadataResponse =
        router.handleGet("/api/metadata");

    assert(metadataResponse.statusCode == 200);
    assert(metadataResponse.contentType == "application/json");
    assert(metadataResponse.body.find("\"metadata\"") != std::string::npos);
    assert(metadataResponse.body.find("\"title\"") != std::string::npos);
    assert(metadataResponse.body.find("\"genre\"") != std::string::npos);

    ApiResponse missingResponse =
        router.handleGet("/api/unknown");

    assert(missingResponse.statusCode == 404);
    assert(missingResponse.contentType == "application/json");
    assert(missingResponse.body.find("\"error\"") != std::string::npos);

    db.close();

    std::cout
        << "test_api_router passed"
        << std::endl;

    return 0;
}
