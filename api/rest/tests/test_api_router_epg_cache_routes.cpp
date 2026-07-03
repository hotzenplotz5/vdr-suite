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
#include "EpgCacheController.h"
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

class FakeEpgCacheController : public IEpgCacheController
{
public:
    int refreshCalls = 0;
    int statusCalls = 0;
    int nowNextCalls = 0;
    int windowCalls = 0;

    std::string lastBackendId;
    std::string lastChannelId;
    std::string lastFromTime;
    std::string lastUntilTime;
    int lastEventLimit = 0;
    VdrEventQuery lastQuery;

    ApiResponse refreshBackendWindow(
        const std::string& backendId,
        const VdrEventQuery& query) override
    {
        ++refreshCalls;
        lastBackendId = backendId;
        lastQuery = query;

        ApiResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = "{\"route\":\"refresh\"}";
        return response;
    }


    ApiResponse getStatus(
        const std::string& backendId) const override
    {
        FakeEpgCacheController* self =
            const_cast<FakeEpgCacheController*>(this);
        ++self->statusCalls;
        self->lastBackendId = backendId;

        ApiResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = "{\"route\":\"status\"}";
        return response;
    }

    ApiResponse getNowNext(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        int eventLimit) const override
    {
        FakeEpgCacheController* self =
            const_cast<FakeEpgCacheController*>(this);
        ++self->nowNextCalls;
        self->lastBackendId = backendId;
        self->lastChannelId = channelId;
        self->lastFromTime = fromTime;
        self->lastEventLimit = eventLimit;

        ApiResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = "{\"route\":\"now-next\"}";
        return response;
    }

    ApiResponse getWindow(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& fromTime,
        const std::string& untilTime,
        int eventLimit) const override
    {
        FakeEpgCacheController* self =
            const_cast<FakeEpgCacheController*>(this);
        ++self->windowCalls;
        self->lastBackendId = backendId;
        self->lastChannelId = channelId;
        self->lastFromTime = fromTime;
        self->lastUntilTime = untilTime;
        self->lastEventLimit = eventLimit;

        ApiResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = "{\"route\":\"window\"}";
        return response;
    }
};

static bool contains(
    const std::string& text,
    const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

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

    ApiRouter unavailableRouter(
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
        liveTransportController);

    const ApiResponse unavailableRefresh = unavailableRouter.handlePost(
        "/api/epg/cache/refresh?backend=home-vdr&channelId=channel-1&from=0&chevents=2",
        "");
    assert(unavailableRefresh.statusCode == 503);
    assert(contains(unavailableRefresh.body, "epg cache unavailable"));

    const ApiResponse unavailableRead = unavailableRouter.handleGet(
        "/api/epg/cache/now-next?backend=home-vdr&channelId=channel-1&fromTime=1000&limit=2");
    assert(unavailableRead.statusCode == 503);
    assert(contains(unavailableRead.body, "epg cache unavailable"));

    const ApiResponse unavailableStatus = unavailableRouter.handleGet(
        "/api/epg/cache/status?backend=home-vdr");
    assert(unavailableStatus.statusCode == 503);
    assert(contains(unavailableStatus.body, "epg cache unavailable"));

    FakeEpgCacheController fakeEpgCacheController;

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
        nullptr,
        &fakeEpgCacheController);

    const ApiResponse refresh = router.handlePost(
        "/api/epg/cache/refresh?backend=home-vdr&channelId=channel-1&from=0&timespan=7200&start=3&limit=4&chevents=2",
        "");

    assert(refresh.statusCode == 200);
    assert(contains(refresh.body, "refresh"));
    assert(fakeEpgCacheController.refreshCalls == 1);
    assert(fakeEpgCacheController.lastBackendId == "home-vdr");
    assert(fakeEpgCacheController.lastQuery.channelId == "channel-1");
    assert(fakeEpgCacheController.lastQuery.from == 0);
    assert(fakeEpgCacheController.lastQuery.timespan == 7200);
    assert(fakeEpgCacheController.lastQuery.start == 3);
    assert(fakeEpgCacheController.lastQuery.limit == 4);
    assert(fakeEpgCacheController.lastQuery.channelEventLimit == 2);

    const ApiResponse status = router.handleGet(
        "/api/epg/cache/status?backend=status-vdr");

    assert(status.statusCode == 200);
    assert(contains(status.body, "status"));
    assert(fakeEpgCacheController.statusCalls == 1);
    assert(fakeEpgCacheController.lastBackendId == "status-vdr");

    const ApiResponse nowNext = router.handleGet(
        "/api/epg/cache/now-next?backend=parents-vdr&channelId=channel-2&fromTime=1000&limit=9");

    assert(nowNext.statusCode == 200);
    assert(contains(nowNext.body, "now-next"));
    assert(fakeEpgCacheController.nowNextCalls == 1);
    assert(fakeEpgCacheController.lastBackendId == "parents-vdr");
    assert(fakeEpgCacheController.lastChannelId == "channel-2");
    assert(fakeEpgCacheController.lastFromTime == "1000");
    assert(fakeEpgCacheController.lastEventLimit == 9);

    const ApiResponse window = router.handleGet(
        "/api/epg/cache/window?channelId=channel-3&fromTime=0900&untilTime=1300&limit=11");

    assert(window.statusCode == 200);
    assert(contains(window.body, "window"));
    assert(fakeEpgCacheController.windowCalls == 1);
    assert(fakeEpgCacheController.lastBackendId == "default");
    assert(fakeEpgCacheController.lastChannelId == "channel-3");
    assert(fakeEpgCacheController.lastFromTime == "0900");
    assert(fakeEpgCacheController.lastUntilTime == "1300");
    assert(fakeEpgCacheController.lastEventLimit == 11);

    std::cout << "test_api_router_epg_cache_routes passed" << std::endl;
    return 0;
}
