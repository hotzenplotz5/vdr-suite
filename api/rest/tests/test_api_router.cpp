#include "ApiRouter.h"
#include "DashboardController.h"
#include "JobsController.h"
#include "MetadataController.h"
#include "RecordingsController.h"
#include "RuntimeDiagnosticsController.h"
#include "VdrController.h"

#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"
#include "Database.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "MetadataRepository.h"
#include "MockVdrAdapter.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrService.h"

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

    MockVdrAdapter adapter;

    VdrService vdrService(adapter);

    VdrOverviewService overviewService(
        vdrService);

    VdrOverviewJsonSerializer vdrJsonSerializer;

    VdrController vdrController(
        overviewService,
        vdrJsonSerializer);

    RuntimeDiagnosticsService runtimeDiagnosticsService;
    RuntimeDiagnosticsJsonSerializer runtimeJsonSerializer;

    RuntimeDiagnosticsController runtimeDiagnosticsController(
        runtimeDiagnosticsService,
        runtimeJsonSerializer);

    ApiRouter router(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController,
        runtimeDiagnosticsController);

    ApiResponse dashboardResponse =
        router.handleGet("/api/dashboard");

    assert(dashboardResponse.statusCode == 200);

    ApiResponse jobsResponse =
        router.handleGet("/api/jobs");

    assert(jobsResponse.statusCode == 200);

    ApiResponse recordingsResponse =
        router.handleGet("/api/recordings");

    assert(recordingsResponse.statusCode == 200);

    ApiResponse metadataResponse =
        router.handleGet("/api/metadata");

    assert(metadataResponse.statusCode == 200);

    ApiResponse vdrResponse =
        router.handleGet("/api/vdr/overview");

    assert(vdrResponse.statusCode == 200);
    assert(vdrResponse.contentType == "application/json");

    assert(vdrResponse.body.find("\"status\"")
           != std::string::npos);

    assert(vdrResponse.body.find("\"channels\"")
           != std::string::npos);

    assert(vdrResponse.body.find("\"recordings\"")
           != std::string::npos);

    ApiResponse runtimeResponse =
        router.handleGet("/api/runtime");

    assert(runtimeResponse.statusCode == 200);
    assert(runtimeResponse.contentType == "application/json");
    assert(runtimeResponse.body == "{\"measurements\":[]}");

    ApiResponse missingResponse =
        router.handleGet("/api/unknown");

    assert(missingResponse.statusCode == 404);

    db.close();

    std::cout
        << "test_api_router passed"
        << std::endl;

    return 0;
}
