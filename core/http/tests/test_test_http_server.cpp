#include "ApiRouter.h"
#include "BackendRegistryService.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryController.h"
#include "BackendRegistry.h"
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
#include "RuntimeDiagnosticsController.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "RuntimeMeasurement.h"
#include "SnapshotAccessService.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedJsonSerializer.h"
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

    snapshot.status.enabled = true;
    snapshot.status.mode = "http-server-snapshot";
    snapshot.status.host = "http-server-host";
    snapshot.status.port = 8002;
    snapshot.status.state = "cached";

    return snapshot;
}

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
    SnapshotCache& snapshotCache,
    SnapshotCacheService& snapshotCacheService,
    SnapshotAccessService& snapshotAccessService,
    VdrSnapshotReadService& snapshotReadService,
    VdrSnapshotReadJsonSerializer& snapshotReadJsonSerializer,
    VdrController& vdrController,
    BackendRegistry& backendRegistry,
    BackendRegistryService& backendRegistryService,
    BackendRegistryJsonSerializer& backendRegistryJsonSerializer,
    BackendRegistryController& backendRegistryController,
    RuntimeDiagnosticsService& runtimeDiagnosticsService,
    RuntimeDiagnosticsJsonSerializer& runtimeJsonSerializer,
    RuntimeDiagnosticsController& runtimeDiagnosticsController,
    SnapshotChangeFeed& snapshotChangeFeed,
    SnapshotChangeFeedJsonSerializer& snapshotChangeFeedJsonSerializer,
    SnapshotChangeFeedController& snapshotChangeFeedController)
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
    (void)snapshotCache;
    (void)snapshotCacheService;
    (void)snapshotAccessService;
    (void)snapshotReadService;
    (void)snapshotReadJsonSerializer;
    (void)backendRegistryJsonSerializer;
    (void)backendRegistryService;
    (void)backendRegistry;
    (void)runtimeDiagnosticsService;
    (void)runtimeJsonSerializer;
    (void)snapshotChangeFeed;
    (void)snapshotChangeFeedJsonSerializer;

    return ApiRouter(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController,
        backendRegistryController,
        runtimeDiagnosticsController,
        snapshotChangeFeedController);
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

    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    snapshotCache.update(makeHttpServerSnapshot());

    VdrSnapshotReadService snapshotReadService(
        snapshotAccessService);

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

    RuntimeMeasurement secondMeasurement;
    secondMeasurement.component = "PollingService";
    secondMeasurement.operation = "Poll cycle";
    secondMeasurement.durationMs = 10;
    secondMeasurement.statusCode = 0;
    secondMeasurement.sizeBytes = 200;
    secondMeasurement.itemCount = 12;

    runtimeDiagnosticsService.recordMeasurement(firstMeasurement);
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
            snapshotCache,
            snapshotCacheService,
            snapshotAccessService,
            snapshotReadService,
            snapshotReadJsonSerializer,
            vdrController,
            runtimeDiagnosticsService,
            runtimeJsonSerializer,
            runtimeDiagnosticsController,
            snapshotChangeFeed,
            snapshotChangeFeedJsonSerializer,
            snapshotChangeFeedController);

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

    HttpServerRequest vdrStatusRequest;
    vdrStatusRequest.method = "GET";
    vdrStatusRequest.path = "/api/vdr/status";
    vdrStatusRequest.headers["Accept"] = "application/json";

    HttpServerResponse vdrStatusResponse =
        server.handleRequest(vdrStatusRequest);

    assert(vdrStatusResponse.statusCode == 200);
    assert(vdrStatusResponse.headers.at("Content-Type") == "application/json");
    assert(vdrStatusResponse.body.find("\"mode\":\"http-server-snapshot\"") != std::string::npos);
    assert(vdrStatusResponse.body.find("\"host\":\"http-server-host\"") != std::string::npos);
    assert(vdrStatusResponse.body.find("\"port\":8002") != std::string::npos);

    HttpServerRequest runtimeRequest;
    runtimeRequest.method = "GET";
    runtimeRequest.path = "/api/runtime";
    runtimeRequest.headers["Accept"] = "application/json";

    HttpServerResponse runtimeResponse =
        server.handleRequest(runtimeRequest);

    assert(runtimeResponse.statusCode == 200);
    assert(runtimeResponse.headers.at("Content-Type") == "application/json");
    assert(runtimeResponse.body.find("\"measurements\"") != std::string::npos);
    assert(runtimeResponse.body.find("\"component\":\"PollingService\"") != std::string::npos);
    assert(runtimeResponse.body.find("\"operation\":\"Poll cycle\"") != std::string::npos);
    assert(runtimeResponse.body.find("\"durationMs\":30") != std::string::npos);
    assert(runtimeResponse.body.find("\"durationMs\":10") != std::string::npos);

    HttpServerRequest runtimeSummaryRequest;
    runtimeSummaryRequest.method = "GET";
    runtimeSummaryRequest.path = "/api/runtime/summary";
    runtimeSummaryRequest.headers["Accept"] = "application/json";

    HttpServerResponse runtimeSummaryResponse =
        server.handleRequest(runtimeSummaryRequest);

    assert(runtimeSummaryResponse.statusCode == 200);
    assert(runtimeSummaryResponse.headers.at("Content-Type") == "application/json");
    assert(runtimeSummaryResponse.body.find("\"summaries\"") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"component\":\"PollingService\"") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"operation\":\"Poll cycle\"") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"count\":2") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"minDurationMs\":10") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"maxDurationMs\":30") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"lastDurationMs\":10") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"lastSizeBytes\":200") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"lastItemCount\":12") != std::string::npos);

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
