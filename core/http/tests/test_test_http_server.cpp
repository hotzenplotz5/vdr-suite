#include "ApiRouter.h"
#include "BackendRegistry.h"
#include "BackendRegistryController.h"
#include "VdrCapabilitySet.h"
#include "CapabilityResolver.h"
#include "CapabilityReportService.h"
#include "CapabilityReportJsonSerializer.h"
#include "CapabilityReportBuilder.h"
#include "CapabilityController.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryService.h"
#include "DashboardController.h"
#include "EpgController.h"
#include "EpgSearchResultJsonSerializer.h"
#include "EpgSearchService.h"
#include "EpgQueryService.h"
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
#include "RecordingActionExecutionController.h"
#include "RecordingActionValidationController.h"
#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationService.h"
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
#include "VdrRecordingQueryController.h"
#include "VdrTimerActionController.h"
#include "VdrTimerActionExecutionService.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "VdrTimerActionRequestParser.h"
#include "VdrTimerActionResultJsonSerializer.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"
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
    EpgQueryService epgQueryService(vdrService);
    EpgSearchService epgSearchService;
    EpgSearchResultJsonSerializer epgSearchResultJsonSerializer;
    EpgController epgController(
        epgQueryService,
        epgSearchService,
        epgSearchResultJsonSerializer);
    VdrOverviewService vdrOverviewService(vdrService);
    VdrOverviewJsonSerializer vdrJsonSerializer;
    VdrRecordingQueryService vdrRecordingQueryService(vdrService);
    VdrRecordingQueryResultJsonSerializer vdrRecordingQueryJsonSerializer;
    VdrRecordingQueryController vdrRecordingQueryController(
        vdrRecordingQueryService,
        vdrRecordingQueryJsonSerializer);

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

    VdrCapabilitySet capabilitySet =
        VdrCapabilitySet::snapshotReadOnly();
    CapabilityResolver capabilityResolver(capabilitySet);
    CapabilityReportBuilder capabilityReportBuilder;
    CapabilityReportService capabilityReportService(
        "default",
        capabilityResolver,
        capabilityReportBuilder);
    CapabilityReportJsonSerializer capabilityReportJsonSerializer;
    CapabilityController capabilityController(
        capabilityReportService,
        capabilityReportJsonSerializer);

    RecordingActionValidationService recordingActionValidationService;
    RecordingActionValidationResultJsonSerializer recordingActionValidationJsonSerializer;
    RecordingActionValidationRequestParser recordingActionValidationRequestParser;
    RecordingActionValidationController recordingActionValidationController(
        recordingActionValidationService,
        recordingActionValidationJsonSerializer,
        recordingActionValidationRequestParser);

    RecordingActionExecutionService recordingActionExecutionService;
    RecordingActionExecutionResultJsonSerializer recordingActionExecutionJsonSerializer;
    RecordingActionBackendExecutorAdapterRegistry recordingActionBackendExecutorAdapterRegistry;
    RecordingActionExecutionController recordingActionExecutionController(
        recordingActionExecutionService,
        recordingActionExecutionJsonSerializer,
        recordingActionBackendExecutorAdapterRegistry,
        recordingActionValidationRequestParser);

    VdrTimerActionExecutionService vdrTimerActionExecutionService;
    VdrTimerActionResultJsonSerializer vdrTimerActionResultJsonSerializer;
    VdrTimerActionRequestParser vdrTimerActionRequestParser;
    VdrTimerActionController vdrTimerActionController(
        vdrTimerActionExecutionService,
        vdrTimerActionResultJsonSerializer,
        vdrTimerActionRequestParser);
    VdrTimerActionExecutorAdapterRegistry vdrTimerActionExecutorAdapterRegistry;

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
        vdrRecordingQueryController,
        snapshotReadService,
        &epgController,
        nullptr,
        nullptr,
        backendRegistryController,
        capabilityController,
        recordingActionValidationController,
        recordingActionExecutionController,
        vdrTimerActionController,
        vdrTimerActionExecutorAdapterRegistry,
        runtimeDiagnosticsController,
        snapshotChangeFeedController,
        nullptr,
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

    HttpServerRequest dashboardQueryRequest;
    dashboardQueryRequest.method = "GET";
    dashboardQueryRequest.path = "/api/dashboard?ignored=true";
    HttpServerResponse dashboardQueryResponse =
        server.handleRequest(dashboardQueryRequest);
    assertJsonResponse(dashboardQueryResponse, 200);
    assert(dashboardQueryResponse.body.find("\"jobs\"") != std::string::npos);

    HttpServerRequest epgQueryRequest;
    epgQueryRequest.method = "GET";
    epgQueryRequest.path = "/api/epg/now-next?channelId=1&from=123";
    HttpServerResponse epgQueryResponse =
        server.handleRequest(epgQueryRequest);
    assertJsonResponse(epgQueryResponse, 200);
    assert(epgQueryResponse.body.find("\"events\"") != std::string::npos);

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
    postRequest.body = "{\"ignored\":true}";
    HttpServerResponse postResponse =
        server.handleRequest(postRequest);
    assertJsonResponse(postResponse, 404);
    assert(postResponse.body == "{\"error\":\"not found\"}");

    HttpServerRequest validationRequest;
    validationRequest.method = "POST";
    validationRequest.path = "/api/recordings/actions/validate";
    validationRequest.body =
        "{"
        "\"backendId\":\"default\","
        "\"recordingId\":\"http-recording-1\","
        "\"action\":\"MOVE\","
        "\"dryRun\":true,"
        "\"targetPath\":\"/srv/vdr/video/archive\""
        "}";
    HttpServerResponse validationResponse =
        server.handleRequest(validationRequest);
    assertJsonResponse(validationResponse, 200);
    assert(validationResponse.body.find("\"valid\":true") != std::string::npos);
    assert(validationResponse.body.find("\"dryRun\":true") != std::string::npos);
    assert(validationResponse.body.find("\"wouldCreateJob\":false") != std::string::npos);
    assert(validationResponse.body.find("\"recordingId\":\"http-recording-1\"") != std::string::npos);
    assert(validationResponse.body.find("\"requiredCapabilities\":[\"recordings.action.move\"]")
           != std::string::npos);
    assert(validationResponse.body.find("\"warnings\":[\"dry-run only\"]")
           != std::string::npos);

    HttpServerRequest vdrValidationRequest;
    vdrValidationRequest.method = "POST";
    vdrValidationRequest.path = "/api/vdr/recordings/actions/validate";
    vdrValidationRequest.body =
        "{"
        "\"backendId\":\"default\","
        "\"recordingId\":\"http-recording-2\","
        "\"action\":\"DELETE\","
        "\"dryRun\":true"
        "}";
    HttpServerResponse vdrValidationResponse =
        server.handleRequest(vdrValidationRequest);
    assertJsonResponse(vdrValidationResponse, 200);
    assert(vdrValidationResponse.body.find("\"valid\":true") != std::string::npos);
    assert(vdrValidationResponse.body.find("\"recordingId\":\"http-recording-2\"") != std::string::npos);
    assert(vdrValidationResponse.body.find("\"requiredCapabilities\":[\"recordings.action.delete\"]")
           != std::string::npos);

    HttpServerRequest unsupportedMethodRequest;
    unsupportedMethodRequest.method = "PUT";
    unsupportedMethodRequest.path = "/api/dashboard";
    HttpServerResponse unsupportedMethodResponse =
        server.handleRequest(unsupportedMethodRequest);
    assertJsonResponse(unsupportedMethodResponse, 405);
    assert(unsupportedMethodResponse.body == "{\"error\":\"method not allowed\"}");

    db.close();

    std::cout
        << "test_test_http_server passed"
        << std::endl;

    return 0;
}
