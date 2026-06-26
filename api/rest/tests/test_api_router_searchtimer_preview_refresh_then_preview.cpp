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
#include "SearchTimerController.h"
#include "SearchTimerPreviewEpgCache.h"
#include "SearchTimerPreviewEpgCacheRefreshController.h"
#include "SearchTimerPreviewEpgCacheRefreshService.h"
#include "SearchTimerPreviewEpgCacheRefreshServiceRegistry.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerService.h"
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
#include "VdrSnapshotBuilder.h"
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
#include <vector>

class PreviewRefreshAdapter final : public IVdrAdapter
{
public:
    mutable int fullEventsReadCount = 0;
    mutable int selectiveEventsReadCount = 0;
    mutable VdrEventQuery lastQuery;

    VdrStatus getStatus() const override
    {
        return VdrStatus();
    }

    std::vector<VdrEvent> getEvents() const override
    {
        ++fullEventsReadCount;
        return {};
    }

    std::vector<VdrEvent> getEvents(const VdrEventQuery& query) const override
    {
        ++selectiveEventsReadCount;
        lastQuery = query;

        VdrEvent event;
        event.id = "cache-event-amerika";
        event.channelId = "cache-channel";
        event.title = "Amerika Cache Dokumentation";
        event.subtitle = "Preview Cache";
        event.description = "This event is loaded only through the refresh cache path.";
        event.startTime = "2026-06-26T20:15:00";
        event.endTime = "2026-06-26T21:00:00";
        event.durationSeconds = 2700;

        return {event};
    }

    std::vector<VdrChannel> getChannels() const override
    {
        return {};
    }

    std::vector<VdrTimer> getTimers() const override
    {
        return {};
    }

    std::vector<VdrRecording> getRecordings() const override
    {
        return {};
    }

    VdrChangeState getChangeState() const override
    {
        return VdrChangeState();
    }
};

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

    PreviewRefreshAdapter adapter;
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

    SearchTimerService searchTimerService;
    SearchTimerResultJsonSerializer searchTimerResultJsonSerializer;
    SearchTimerController searchTimerController(
        searchTimerService,
        searchTimerResultJsonSerializer);

    SearchTimerPreviewEpgCache previewCache;
    VdrSnapshotBuilder snapshotBuilder(
        vdrService,
        "default",
        nullptr,
        nullptr);
    SearchTimerPreviewEpgCacheRefreshService refreshService(
        previewCache,
        snapshotBuilder);
    SearchTimerPreviewEpgCacheRefreshServiceRegistry refreshRegistry;
    refreshRegistry.registerService("default", refreshService);
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
        &searchTimerController,
        liveTransportController,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &refreshController);

    router.setSearchTimerPreviewEpgCache(&previewCache);

    const ApiResponse refreshResponse = router.handlePost(
        "/api/searchtimers/preview/cache/refresh?backend=default&from=100&timespan=7200&channelEventLimit=33",
        "");

    assert(refreshResponse.statusCode == 200);
    assert(refreshResponse.contentType == "application/json");
    assert(refreshResponse.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(refreshResponse.body.find("\"status\":\"ready\"") != std::string::npos);
    assert(refreshResponse.body.find("\"available\":true") != std::string::npos);
    assert(refreshResponse.body.find("\"eventCount\":1") != std::string::npos);

    assert(adapter.fullEventsReadCount == 0);
    assert(adapter.selectiveEventsReadCount == 1);
    assert(adapter.lastQuery.from == 100);
    assert(adapter.lastQuery.timespan == 7200);
    assert(adapter.lastQuery.channelEventLimit == 33);

    const ApiResponse previewResponse = router.handleGet(
        "/api/searchtimers/preview?backend=default&query=Amerika&limit=10");

    assert(previewResponse.statusCode == 200);
    assert(previewResponse.contentType == "application/json");
    assert(previewResponse.body.find("\"epgInput\":{") != std::string::npos);
    assert(previewResponse.body.find("\"status\":\"ready\"") != std::string::npos);
    assert(previewResponse.body.find("\"available\":true") != std::string::npos);
    assert(previewResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(previewResponse.body.find("Amerika Cache Dokumentation") != std::string::npos);

    assert(adapter.fullEventsReadCount == 0);
    assert(adapter.selectiveEventsReadCount == 1);

    std::cout << "test_api_router_searchtimer_preview_refresh_then_preview passed" << std::endl;
    return 0;
}
