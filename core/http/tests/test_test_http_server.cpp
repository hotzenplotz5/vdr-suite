#include "ApiRouter.h"
#include "DashboardController.h"
#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"
#include "Database.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "JobsController.h"
#include "MetadataController.h"
#include "MetadataRepository.h"
#include "MockVdrAdapter.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"
#include "RecordingsController.h"
#include "TestHttpServer.h"
#include "VdrController.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrService.h"

#include <cassert>
#include <iostream>
#include <string>

static ApiRouter createRouter(
    Database& db,
    MockVdrAdapter& adapter,
    JobRepository& jobRepository,
    RecordingRepository& recordingRepository,
    MetadataRepository& metadataRepository,
    JobDashboardService& jobDashboardService,
    RecordingDashboardService& recordingDashboardService,
    DashboardFacade& dashboardFacade,
    DashboardJsonSerializer& dashboardJsonSerializer,
    DashboardController& dashboardController,
    JobsController& jobsController,
    RecordingsController& recordingsController,
    MetadataController& metadataController,
    VdrService& vdrService,
    VdrOverviewService& vdrOverviewService,
    VdrOverviewJsonSerializer& vdrJsonSerializer,
    VdrController& vdrController)
{
    (void)db;
    (void)adapter;
    (void)jobRepository;
    (void)recordingRepository;
    (void)metadataRepository;
    (void)jobDashboardService;
    (void)recordingDashboardService;
    (void)dashboardFacade;
    (void)dashboardJsonSerializer;
    (void)vdrService;
    (void)vdrOverviewService;
    (void)vdrJsonSerializer;

    return ApiRouter(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController);
}

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

    DashboardJsonSerializer dashboardJsonSerializer;

    DashboardController dashboardController(
        dashboardFacade,
        dashboardJsonSerializer);

    JobsController jobsController(jobRepository);

    RecordingsController recordingsController(
        recordingRepository);

    MetadataController metadataController(
        metadataRepository);

    MockVdrAdapter adapter;

    VdrService vdrService(adapter);

    VdrOverviewService vdrOverviewService(
        vdrService);

    VdrOverviewJsonSerializer vdrJsonSerializer;

    VdrController vdrController(
        vdrOverviewService,
        vdrJsonSerializer);

    ApiRouter router =
        createRouter(
            db,
            adapter,
            jobRepository,
            recordingRepository,
            metadataRepository,
            jobDashboardService,
            recordingDashboardService,
            dashboardFacade,
            dashboardJsonSerializer,
            dashboardController,
            jobsController,
            recordingsController,
            metadataController,
            vdrService,
            vdrOverviewService,
            vdrJsonSerializer,
            vdrController);

    TestHttpServer server(router);

    HttpServerRequest dashboardRequest;
    dashboardRequest.method = "GET";
    dashboardRequest.path = "/api/dashboard";
    dashboardRequest.headers["Accept"] = "application/json";

    HttpServerResponse dashboardResponse =
        server.handleRequest(dashboardRequest);

    assert(dashboardResponse.statusCode == 200);
    assert(dashboardResponse.headers.at("Content-Type") == "application/json");
    assert(dashboardResponse.body.find("\"jobs\"") != std::string::npos);
    assert(dashboardResponse.body.find("\"recordings\"") != std::string::npos);

    HttpServerRequest vdrRequest;
    vdrRequest.method = "GET";
    vdrRequest.path = "/api/vdr/overview";
    vdrRequest.headers["Accept"] = "application/json";

    HttpServerResponse vdrResponse =
        server.handleRequest(vdrRequest);

    assert(vdrResponse.statusCode == 200);
    assert(vdrResponse.headers.at("Content-Type") == "application/json");
    assert(vdrResponse.body.find("\"status\"") != std::string::npos);
    assert(vdrResponse.body.find("\"channels\"") != std::string::npos);
    assert(vdrResponse.body.find("\"recordings\"") != std::string::npos);

    HttpServerRequest missingRequest;
    missingRequest.method = "GET";
    missingRequest.path = "/api/missing";

    HttpServerResponse missingResponse =
        server.handleRequest(missingRequest);

    assert(missingResponse.statusCode == 404);
    assert(missingResponse.headers.at("Content-Type") == "application/json");
    assert(missingResponse.body == "{\"error\":\"not found\"}");

    HttpServerRequest postRequest;
    postRequest.method = "POST";
    postRequest.path = "/api/dashboard";

    HttpServerResponse postResponse =
        server.handleRequest(postRequest);

    assert(postResponse.statusCode == 405);
    assert(postResponse.headers.at("Content-Type") == "application/json");
    assert(postResponse.body == "{\"error\":\"method not allowed\"}");

    db.close();

    std::cout
        << "test_test_http_server passed"
        << std::endl;

    return 0;
}
