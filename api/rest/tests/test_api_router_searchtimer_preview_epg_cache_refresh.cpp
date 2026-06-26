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
#include "EpgController.h"
#include "EpgQueryService.h"
#include "EpgSearchResultJsonSerializer.h"
#include "EpgSearchService.h"
#include "IVdrAdapter.h"
#include "JobsController.h"
#include "LiveTransportController.h"
#include "MetadataController.h"
#include "MetadataRepository.h"
#include "MockVdrTimerActionExecutor.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionExecutionController.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationController.h"
#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionValidationService.h"
#include "RecordingDashboardService.h"
#include "RecordingPersonSearchController.h"
#include "RecordingPersonSearchResultJsonSerializer.h"
#include "RecordingPersonSearchService.h"
#include "RecordingRepository.h"
#include "RecordingsController.h"
#include "RuntimeDiagnosticsController.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "SearchTimerAutomationDryRunResultJsonSerializer.h"
#include "SearchTimerAutomationPreviewController.h"
#include "SearchTimerAutomationReadOnlyService.h"
#include "SearchTimerController.h"
#include "SearchTimerDiscoveryController.h"
#include "SearchTimerDiscoveryJsonSerializer.h"
#include "SearchTimerDiscoveryService.h"
#include "SearchTimerDiscoveryStaticProvider.h"
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
#include <memory>
#include <string>
#include <vector>

class RouterPreviewRefreshAdapter final : public IVdrAdapter
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
        event.id = "router-refresh-event";
        event.title = "Router Refresh Event";
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

class RouterPreviewRefreshVdrTimerActionExecutorAdapter final
    : public IVdrTimerActionExecutorAdapter
{
public:
    std::string backendId() const override
    {
        return "default";
    }

    IVdrTimerActionExecutor& executor() override
    {
        return executor_;
    }

private:
    MockVdrTimerActionExecutor executor_;
};

class RouterPreviewRefreshFixture
{
public:
    RouterPreviewRefreshFixture()
        : jobRepository(database),
          recordingRepository(database),
          metadataRepository(database),
          jobDashboardService(jobRepository),
          recordingDashboardService(recordingRepository, metadataRepository),
          dashboardFacade(jobDashboardService, recordingDashboardService),
          dashboardController(dashboardFacade, dashboardJsonSerializer),
          jobsController(jobRepository),
          recordingsController(recordingRepository),
          metadataController(metadataRepository),
          vdrService(adapter),
          epgQueryService(vdrService),
          epgController(epgQueryService, epgSearchService, epgSearchResultJsonSerializer),
          overviewService(vdrService),
          snapshotCacheService(snapshotCache),
          snapshotAccessService(snapshotCacheService),
          snapshotReadService(snapshotAccessService),
          vdrController(overviewService, vdrOverviewJsonSerializer, snapshotReadService, snapshotReadJsonSerializer),
          vdrRecordingQueryService(vdrService),
          vdrRecordingQueryController(vdrRecordingQueryService, vdrRecordingQueryJsonSerializer),
          backendRegistryService(backendRegistry),
          backendRegistryController(backendRegistryService, backendRegistryJsonSerializer),
          capabilityResolver(VdrCapabilitySet::snapshotReadOnly()),
          capabilityReportService("default", capabilityResolver, capabilityReportBuilder),
          capabilityController(capabilityReportService, capabilityReportJsonSerializer),
          recordingActionValidationController(recordingActionValidationService, recordingActionValidationJsonSerializer, recordingActionValidationRequestParser),
          recordingActionExecutionController(recordingActionExecutionService, recordingActionExecutionJsonSerializer, recordingActionBackendExecutorAdapterRegistry, recordingActionValidationRequestParser),
          vdrTimerActionController(vdrTimerActionExecutionService, vdrTimerActionResultJsonSerializer, vdrTimerActionRequestParser),
          runtimeDiagnosticsController(runtimeDiagnosticsService, runtimeDiagnosticsJsonSerializer),
          snapshotChangeFeedController(snapshotChangeFeed, snapshotChangeFeedJsonSerializer),
          searchTimerController(searchTimerService, searchTimerResultJsonSerializer),
          searchTimerDiscoveryController(searchTimerDiscoveryService, searchTimerDiscoveryJsonSerializer),
          searchTimerAutomationPreviewController(searchTimerAutomationReadOnlyService, searchTimerAutomationDryRunResultJsonSerializer),
          liveTransportController(liveTransport),
          snapshotBuilder(vdrService, "default", nullptr, nullptr),
          refreshService(previewCache, snapshotBuilder),
          refreshController(refreshRegistry),
          router(dashboardController,
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
                 &searchTimerController,
                 liveTransportController,
                 nullptr,
                 nullptr,
                 nullptr,
                 &searchTimerDiscoveryController,
                 &searchTimerAutomationPreviewController,
                 &refreshController)
    {
        database.open("/tmp/vdr-suite-test.db");

        BackendNode backend;
        backend.backendId = "default";
        backend.backendName = "Default VDR";
        backend.backendType = "restfulapi";
        backend.enabled = true;
        backend.online = true;
        backendRegistry.addBackend(backend);

        refreshRegistry.registerService("default", refreshService);
        vdrTimerActionExecutorAdapterRegistry.registerAdapter(
            std::make_shared<RouterPreviewRefreshVdrTimerActionExecutorAdapter>());
    }

    ~RouterPreviewRefreshFixture()
    {
        database.close();
    }

    Database database;
    JobRepository jobRepository;
    RecordingRepository recordingRepository;
    MetadataRepository metadataRepository;
    JobDashboardService jobDashboardService;
    RecordingDashboardService recordingDashboardService;
    DashboardFacade dashboardFacade;
    DashboardJsonSerializer dashboardJsonSerializer;
    DashboardController dashboardController;
    JobsController jobsController;
    RecordingsController recordingsController;
    MetadataController metadataController;
    RouterPreviewRefreshAdapter adapter;
    VdrService vdrService;
    EpgQueryService epgQueryService;
    EpgSearchService epgSearchService;
    EpgSearchResultJsonSerializer epgSearchResultJsonSerializer;
    EpgController epgController;
    VdrOverviewService overviewService;
    VdrOverviewJsonSerializer vdrOverviewJsonSerializer;
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService;
    SnapshotAccessService snapshotAccessService;
    VdrSnapshotReadService snapshotReadService;
    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;
    VdrController vdrController;
    VdrRecordingQueryService vdrRecordingQueryService;
    VdrRecordingQueryResultJsonSerializer vdrRecordingQueryJsonSerializer;
    VdrRecordingQueryController vdrRecordingQueryController;
    BackendRegistry backendRegistry;
    BackendRegistryService backendRegistryService;
    BackendRegistryJsonSerializer backendRegistryJsonSerializer;
    BackendRegistryController backendRegistryController;
    CapabilityResolver capabilityResolver;
    CapabilityReportBuilder capabilityReportBuilder;
    CapabilityReportService capabilityReportService;
    CapabilityReportJsonSerializer capabilityReportJsonSerializer;
    CapabilityController capabilityController;
    RecordingActionValidationService recordingActionValidationService;
    RecordingActionValidationResultJsonSerializer recordingActionValidationJsonSerializer;
    RecordingActionValidationRequestParser recordingActionValidationRequestParser;
    RecordingActionValidationController recordingActionValidationController;
    RecordingActionExecutionService recordingActionExecutionService;
    RecordingActionExecutionResultJsonSerializer recordingActionExecutionJsonSerializer;
    RecordingActionBackendExecutorAdapterRegistry recordingActionBackendExecutorAdapterRegistry;
    RecordingActionExecutionController recordingActionExecutionController;
    VdrTimerActionExecutionService vdrTimerActionExecutionService;
    VdrTimerActionResultJsonSerializer vdrTimerActionResultJsonSerializer;
    VdrTimerActionRequestParser vdrTimerActionRequestParser;
    VdrTimerActionController vdrTimerActionController;
    VdrTimerActionExecutorAdapterRegistry vdrTimerActionExecutorAdapterRegistry;
    RuntimeDiagnosticsService runtimeDiagnosticsService;
    RuntimeDiagnosticsJsonSerializer runtimeDiagnosticsJsonSerializer;
    RuntimeDiagnosticsController runtimeDiagnosticsController;
    SnapshotChangeFeed snapshotChangeFeed;
    SnapshotChangeFeedJsonSerializer snapshotChangeFeedJsonSerializer;
    SnapshotChangeFeedController snapshotChangeFeedController;
    SearchTimerService searchTimerService;
    SearchTimerResultJsonSerializer searchTimerResultJsonSerializer;
    SearchTimerController searchTimerController;
    SearchTimerDiscoveryStaticProvider searchTimerDiscoveryProvider;
    SearchTimerDiscoveryService searchTimerDiscoveryService{searchTimerDiscoveryProvider};
    SearchTimerDiscoveryJsonSerializer searchTimerDiscoveryJsonSerializer;
    SearchTimerDiscoveryController searchTimerDiscoveryController;
    SearchTimerAutomationReadOnlyService searchTimerAutomationReadOnlyService;
    SearchTimerAutomationDryRunResultJsonSerializer searchTimerAutomationDryRunResultJsonSerializer;
    SearchTimerAutomationPreviewController searchTimerAutomationPreviewController;
    SseLiveTransport liveTransport;
    LiveTransportController liveTransportController;
    SearchTimerPreviewEpgCache previewCache;
    VdrSnapshotBuilder snapshotBuilder;
    SearchTimerPreviewEpgCacheRefreshService refreshService;
    SearchTimerPreviewEpgCacheRefreshServiceRegistry refreshRegistry;
    SearchTimerPreviewEpgCacheRefreshController refreshController;
    ApiRouter router;
};

static void test_router_refresh_route_uses_query_parameters()
{
    RouterPreviewRefreshFixture fixture;

    const ApiResponse response = fixture.router.handlePost(
        "/api/searchtimers/preview/cache/refresh?backend=default&from=100&timespan=7200&start=5&limit=6&channelEventLimit=33",
        "");

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(response.body.find("\"status\":\"ready\"") != std::string::npos);
    assert(response.body.find("\"available\":true") != std::string::npos);
    assert(response.body.find("\"eventCount\":1") != std::string::npos);

    assert(fixture.adapter.fullEventsReadCount == 0);
    assert(fixture.adapter.selectiveEventsReadCount == 1);
    assert(fixture.adapter.lastQuery.from == 100);
    assert(fixture.adapter.lastQuery.timespan == 7200);
    assert(fixture.adapter.lastQuery.start == 5);
    assert(fixture.adapter.lastQuery.limit == 6);
    assert(fixture.adapter.lastQuery.channelEventLimit == 33);
}

static void test_router_refresh_alias_supports_chevents_and_default_backend()
{
    RouterPreviewRefreshFixture fixture;

    const ApiResponse response = fixture.router.handlePost(
        "/api/vdr/searchtimers/preview/cache/refresh?chevents=17",
        "");

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(response.body.find("\"status\":\"ready\"") != std::string::npos);

    assert(fixture.adapter.fullEventsReadCount == 0);
    assert(fixture.adapter.selectiveEventsReadCount == 1);
    assert(fixture.adapter.lastQuery.channelEventLimit == 17);
}

static void test_router_refresh_route_reports_unknown_backend()
{
    RouterPreviewRefreshFixture fixture;

    const ApiResponse response = fixture.router.handlePost(
        "/api/searchtimers/preview/cache/refresh?backend=missing",
        "");

    assert(response.statusCode == 404);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"missing\"") != std::string::npos);
    assert(response.body.find("\"status\":\"backend-not-found\"") != std::string::npos);
    assert(response.body.find("\"available\":false") != std::string::npos);
    assert(response.body.find("\"eventCount\":0") != std::string::npos);

    assert(fixture.adapter.fullEventsReadCount == 0);
    assert(fixture.adapter.selectiveEventsReadCount == 0);
}

int main()
{
    test_router_refresh_route_uses_query_parameters();
    test_router_refresh_alias_supports_chevents_and_default_backend();
    test_router_refresh_route_reports_unknown_backend();

    std::cout << "test_api_router_searchtimer_preview_epg_cache_refresh passed" << std::endl;
    return 0;
}
