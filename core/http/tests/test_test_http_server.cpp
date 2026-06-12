#include "ApiRouter.h"
#include "BackendRegistry.h"
#include "BackendRegistryController.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryService.h"
#include "DashboardController.h"
#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"
#include "Database.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "JobsController.h"
#include "LiveTransportController.h"
#include "MetadataController.h"
#include "MetadataRepository.h"
#include "MockVdrAdapter.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"
#include "RecordingsController.h"
#include "RuntimeDiagnosticsController.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "RuntimeMeasurement.h"
#include "SnapshotAccessService.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedJsonSerializer.h"
#include "SseLiveTransport.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "TestHttpServer.h"
#include "VdrController.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrSnapshotReadService.h"

#include <cassert>
#include <iostream>
#include <string>

static VdrSnapshot makeHttpServerSnapshot()
{
    VdrSnapshot snapshot;
    snapshot.backendId = "default";
    snapshot.status.enabled = true;
    snapshot.status.mode = "http-server-snapshot";
    snapshot.status.host = "http-server-host";
    snapshot.status.port = 8002;
    snapshot.status.state = "cached";
    return snapshot;
}

static void assertJsonResponse(
    const HttpServerResponse& response,
    int expectedStatusCode)
{
    assert(response.statusCode == expectedStatusCode);
    assert(response.headers.at("Content-Type") == "application/json");
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
    RecordingsController recordingsController(recordingRepository);
    MetadataController metadataController(metadataRepository);

    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService vdrOverviewService(vdrService);
    VdrOverviewJsonSerializer vdrJsonSerializer;

    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    snapshotCache.update(makeHttpServerSnapshot());

    VdrSnapshotReadService snapshotReadService(snapshotAccessService);
    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;
    VdrController vdrController(
        vdrOverviewService,
        vdrJsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);

    BackendRegistry backendRegistry;
    BackendNode defaultBackend;
    defaultBackend.backendId = "default";
    defaultBackend.backendName = "Default VDR";
    defaultBackend.backendType = "vdr";
    defaultBackend.enabled = true;
    defaultBackend.online = false;
    backendRegistry.addBackend(defaultBackend);

    BackendRegistryService backendRegistryService(backendRegistry);
    BackendRegistryJsonSerializer backendRegistryJsonSerializer;
    BackendRegistryController backendRegistryController(
        backendRegistryService,
        backendRegistryJsonSerializer);

    RuntimeDiagnosticsService runtimeDiagnosticsService;
    RuntimeDiagnosticsJsonSerializer runtimeJsonSerializer;

    RuntimeMeasurement firstMeasurement;
    firstMeasurement.component = "PollingService";
    firstMeasurement.operation = "Poll cycle";
    firstMeasurement.durationMs = 30;
    firstMeasurement.statusCode = 0;
    firstMeasurement.sizeBytes = 100;
    firstMeasurement.itemCount = 4;
    runtimeDiagnosticsService.recordMeasurement(firstMeasurement);

    RuntimeMeasurement secondMeasurement;
    secondMeasurement.component = "PollingService";
    secondMeasurement.operation = "Poll cycle";
    secondMeasurement.durationMs = 10;
    secondMeasurement.statusCode = 0;
    secondMeasurement.sizeBytes = 200;
    secondMeasurement.itemCount = 12;
    runtimeDiagnosticsService.recordMeasurement(secondMeasurement);

    RuntimeDiagnosticsController runtimeDiagnosticsController(
        runtimeDiagnosticsService,
        runtimeJsonSerializer);

    SnapshotChangeFeed snapshotChangeFeed;
    snapshotChangeFeed.addEntry(SnapshotChangeFeedEntry(3, 1, {"status"}));
    SnapshotChangeFeedJsonSerializer snapshotChangeFeedJsonSerializer;
    SnapshotChangeFeedController snapshotChangeFeedController(
        snapshotChangeFeed,
        snapshotChangeFeedJsonSerializer);

    SseLiveTransport liveTransport;
    LiveTransportController liveTransportController(liveTransport);

    ApiRouter router(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController,
        backendRegistryController,
        runtimeDiagnosticsController,
        snapshotChangeFeedController,
        liveTransportController);

    TestHttpServer server(router);

    HttpServerRequest dashboardRequest;
    dashboardRequest.method = "GET";
    dashboardRequest.path = "/api/dashboard";
    HttpServerResponse dashboardResponse =
        server.handleRequest(dashboardRequest);
    assertJsonResponse(dashboardResponse, 200);
    assert(dashboardResponse.body.find("\"jobs\"") != std::string::npos);
    assert(dashboardResponse.body.find("\"recordings\"") != std::string::npos);

    HttpServerRequest vdrStatusRequest;
    vdrStatusRequest.method = "GET";
    vdrStatusRequest.path = "/api/vdr/status";
    HttpServerResponse vdrStatusResponse =
        server.handleRequest(vdrStatusRequest);
    assertJsonResponse(vdrStatusResponse, 200);
    assert(vdrStatusResponse.body.find("\"mode\":\"http-server-snapshot\"") != std::string::npos);
    assert(vdrStatusResponse.body.find("\"host\":\"http-server-host\"") != std::string::npos);

    HttpServerRequest backendHealthRequest;
    backendHealthRequest.method = "GET";
    backendHealthRequest.path = "/api/backends/default/health";
    HttpServerResponse backendHealthResponse =
        server.handleRequest(backendHealthRequest);
    assertJsonResponse(backendHealthResponse, 200);
    assert(backendHealthResponse.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(backendHealthResponse.body.find("\"snapshotAvailable\":true") != std::string::npos);

    HttpServerRequest runtimeSummaryRequest;
    runtimeSummaryRequest.method = "GET";
    runtimeSummaryRequest.path = "/api/runtime/summary";
    HttpServerResponse runtimeSummaryResponse =
        server.handleRequest(runtimeSummaryRequest);
    assertJsonResponse(runtimeSummaryResponse, 200);
    assert(runtimeSummaryResponse.body.find("\"summaries\"") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"count\":2") != std::string::npos);

    HttpServerRequest missingRequest;
    missingRequest.method = "GET";
    missingRequest.path = "/api/missing";
    HttpServerResponse missingResponse =
        server.handleRequest(missingRequest);
    assertJsonResponse(missingResponse, 404);
    assert(missingResponse.body == "{\"error\":\"not found\"}");

    HttpServerRequest postRequest;
    postRequest.method = "POST";
    postRequest.path = "/api/dashboard";
    HttpServerResponse postResponse =
        server.handleRequest(postRequest);
    assertJsonResponse(postResponse, 405);
    assert(postResponse.body == "{\"error\":\"method not allowed\"}");

    db.close();

    std::cout
        << "test_test_http_server passed"
        << std::endl;

    return 0;
}
