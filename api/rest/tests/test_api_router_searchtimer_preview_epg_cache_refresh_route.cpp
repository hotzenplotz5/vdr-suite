#include "ApiRouter.h"
#include "BackendRegistry.h"
#include "BackendRegistryController.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryService.h"
#include "CapabilityController.h"
#include "CapabilityReportBuilder.h"
#include "CapabilityReportJsonSerializer.h"
#include "CapabilityReportService.h"
#include "CapabilityResolver.h"
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
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionExecutionController.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationController.h"
#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionValidationService.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"
#include "RecordingsController.h"
#include "RuntimeDiagnosticsController.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "SearchTimerPreviewEpgCacheRefreshController.h"
#include "SearchTimerPreviewEpgCacheRefreshServiceRegistry.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedJsonSerializer.h"
#include "SseLiveTransport.h"
#include "VdrCapabilitySet.h"
#include "VdrController.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrRecordingQueryController.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrSnapshotReadService.h"
#include "VdrTimerActionController.h"
#include "VdrTimerActionExecutionService.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "VdrTimerActionRequestParser.h"
#include "VdrTimerActionResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db")) {
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

    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);

    VdrSnapshot snapshot;
    snapshot.backendId = "default";
    snapshotCache.update(snapshot);

    VdrSnapshotReadService snapshotReadService(snapshotAccessService);
    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;

    VdrOverviewService vdrOverviewService(vdrService);
    VdrOverviewJsonSerializer vdrOverviewJsonSerializer;
    VdrController vdrController(
        vdrOverviewService,
        vdrOverviewJsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);

    VdrRecordingQueryService vdrRecordingQueryService(vdrService);
    VdrRecordingQueryResultJsonSerializer vdrRecordingQueryJsonSerializer;
    VdrRecordingQueryController vdrRecordingQueryController(
        vdrRecordingQueryService,
        vdrRecordingQueryJsonSerializer);

    BackendRegistry backendRegistry;
    BackendNode backend;
    backend.backendId = "default";
    backend.backendName = "Default VDR";
    backend.backendType = "vdr";
    backend.enabled = true;
    backend.online = true;
    backend.capabilities = VdrCapabilitySet::snapshotReadOnly();
    backendRegistry.addBackend(backend);

    BackendRegistryService backendRegistryService(backendRegistry);
    BackendRegistryJsonSerializer backendRegistryJsonSerializer;
    BackendRegistryController backendRegistryController(
        backendRegistryService,
        backendRegistryJsonSerializer);

    VdrCapabilitySet capabilitySet = VdrCapabilitySet::snapshotReadOnly();
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
    RuntimeDiagnosticsJsonSerializer runtimeDiagnosticsJsonSerializer;
    RuntimeDiagnosticsController runtimeDiagnosticsController(
        runtimeDiagnosticsService,
        runtimeDiagnosticsJsonSerializer);

    SnapshotChangeFeed snapshotChangeFeed;
    SnapshotChangeFeedJsonSerializer snapshotChangeFeedJsonSerializer;
    SnapshotChangeFeedController snapshotChangeFeedController(
        snapshotChangeFeed,
        snapshotChangeFeedJsonSerializer);

    SseLiveTransport liveTransport;
    LiveTransportController liveTransportController(liveTransport);

    SearchTimerPreviewEpgCacheRefreshServiceRegistry refreshRegistry;
    SearchTimerPreviewEpgCacheRefreshController refreshController(
        refreshRegistry);

    ApiRouter router(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController,
        vdrRecordingQueryController,
        snapshotReadService,
        nullptr,
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
        liveTransportController,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &refreshController);

    const ApiResponse response = router.handlePost(
        "/api/searchtimers/preview/cache/refresh?backend=missing&from=100&timespan=7200&channelEventLimit=33",
        "");

    assert(response.statusCode == 404);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"missing\"") != std::string::npos);
    assert(response.body.find("\"status\":\"backend-not-found\"") != std::string::npos);
    assert(response.body.find("\"available\":false") != std::string::npos);
    assert(response.body.find("\"eventCount\":0") != std::string::npos);

    const ApiResponse aliasResponse = router.handlePost(
        "/api/vdr/searchtimers/preview/cache/refresh?backend=missing",
        "");

    assert(aliasResponse.statusCode == 404);
    assert(aliasResponse.contentType == "application/json");
    assert(aliasResponse.body.find("\"status\":\"backend-not-found\"") != std::string::npos);

    std::cout << "test_api_router_searchtimer_preview_epg_cache_refresh_route passed" << std::endl;
    return 0;
}
