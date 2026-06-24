#include "ApiRouter.h"
#include "BackendRegistryService.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryController.h"
#include "VdrCapabilitySet.h"
#include "CapabilityResolver.h"
#include "CapabilityReportService.h"
#include "CapabilityReportJsonSerializer.h"
#include "CapabilityReportBuilder.h"
#include "CapabilityController.h"
#include "BackendRegistry.h"
#include "DashboardController.h"
#include "EpgController.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyOperatorRefreshController.h"
#include "EpgSearchNativeFuzzyOperatorRefreshService.h"
#include "EpgSearchResultJsonSerializer.h"
#include "EpgSearchService.h"
#include "EpgQueryService.h"
#include "JobsController.h"
#include "LiveTransportController.h"
#include "MetadataController.h"
#include "ISearchTimerCommandExecutor.h"
#include "ISearchTimerDataSource.h"
#include "PersonSearchService.h"
#include "PersonResolutionJsonSerializer.h"
#include "PersonQueryResultJsonSerializer.h"
#include "PersonController.h"
#include "RecordingsController.h"
#include "RecordingActionExecutionController.h"
#include "RecordingActionValidationController.h"
#include "RecordingPersonSearchService.h"
#include "RecordingPersonSearchResultJsonSerializer.h"
#include "RecordingPersonSearchController.h"
#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationService.h"
#include "RuntimeDiagnosticsController.h"
#include "SearchTimerController.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerService.h"
#include "SnapshotChangeFeedController.h"
#include "VdrController.h"
#include "VdrRecordingQueryService.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryController.h"
#include "VdrTimerActionController.h"
#include "VdrTimerActionExecutionService.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "VdrTimerActionRequestParser.h"
#include "VdrTimerActionResultJsonSerializer.h"
#include "MockVdrTimerActionExecutor.h"

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
#include "RuntimeMeasurement.h"
#include "SnapshotAccessService.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedJsonSerializer.h"
#include "SseLiveTransport.h"
#include "LiveUpdateEvent.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrSnapshotReadService.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>


namespace
{
class RouterRecordingActionBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    explicit RouterRecordingActionBackendExecutorAdapter(
        std::string backendIdValue)
        : backendIdValue_(backendIdValue)
    {
    }

    std::string backendId() const override
    {
        return backendIdValue_;
    }

    std::string backendType() const override
    {
        return "router-test-backend";
    }

    RecordingActionCapabilitySet capabilities() const override
    {
        RecordingActionCapabilityContract contract;
        return contract.restfulApiDefaultCapabilities();
    }

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        RecordingActionExecutionResult result;
        result.success = !payload.dryRun;
        result.type = payload.type;
        result.backendId = payload.backendId;
        result.recordingId = payload.recordingId;

        if (payload.dryRun)
        {
            result.message = "router dry-run execution skipped";
        }
        else
        {
            result.message = "router backend execution completed";
        }

        return result;
    }

private:
    std::string backendIdValue_;
};
}


class RouterSearchTimerDataSource final : public ISearchTimerDataSource
{
public:
    RouterSearchTimerDataSource()
    {
        timers_.push_back(SearchTimer::create(
            SearchTimerId::fromBackendNativeId("default", "1"),
            "Router SearchTimer Terra X",
            "Terra X",
            SearchTimerState::Active));
        timers_.push_back(SearchTimer::create(
            SearchTimerId::fromBackendNativeId("ferienhaus", "2"),
            "Router SearchTimer Inactive",
            "Inactive",
            SearchTimerState::Inactive));
    }

    SearchTimerResult list(
        const SearchTimerQuery& query) const override
    {
        SearchTimerService service;
        return service.list(timers_, query);
    }

private:
    std::vector<SearchTimer> timers_;
};

class RouterNativeFuzzySearchTimerRuntime final
    : public ISearchTimerCommandExecutor,
      public ISearchTimerDataSource
{
public:
    int createCalls = 0;
    int deleteCalls = 0;
    std::vector<SearchTimer> timers;

    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++createCalls;

        SearchTimer timer = SearchTimer::create(
            SearchTimerId::fromBackendNativeId(
                request.backendId,
                "router-native-fuzzy-probe"),
            request.name,
            request.query,
            SearchTimerState::Active);

        timer.matchOptions().setMode(request.matchMode);
        timer.matchOptions().setTolerance(request.matchTolerance);

        timers.push_back(timer);

        return SearchTimerCreateResult::ok(
            timer,
            "router native fuzzy probe created");
    }

    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) override
    {
        (void)request;
        return SearchTimerUpdateResult::failed(
            "not implemented",
            {"not implemented"});
    }

    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request) override
    {
        ++deleteCalls;

        timers.erase(
            std::remove_if(
                timers.begin(),
                timers.end(),
                [&](const SearchTimer& timer) {
                    return timer.backendNativeId()
                        == request.backendNativeId;
                }),
            timers.end());

        return SearchTimerDeleteResult::ok(
            request.backendId,
            request.backendNativeId,
            "router native fuzzy probe deleted");
    }

    SearchTimerResult list(
        const SearchTimerQuery& query) const override
    {
        SearchTimerService service;
        return service.list(
            timers,
            query);
    }
};

class RouterVdrTimerActionExecutorAdapter final
    : public IVdrTimerActionExecutorAdapter
{
public:
    explicit RouterVdrTimerActionExecutorAdapter(
        std::string backendIdValue)
        : backendIdValue_(backendIdValue)
    {
    }

    std::string backendId() const override
    {
        return backendIdValue_;
    }

    IVdrTimerActionExecutor& executor() override
    {
        return executor_;
    }

private:
    std::string backendIdValue_;
    MockVdrTimerActionExecutor executor_;
};

static VdrSnapshot makeRouterSnapshot()
{
    VdrSnapshot snapshot;

    snapshot.status.enabled = true;
    snapshot.status.mode = "router-snapshot";
    snapshot.status.host = "router-host";
    snapshot.status.port = 8002;
    snapshot.status.state = "cached";

    VdrChannel channel;
    channel.id = "router-channel-1";
    channel.number = 1;
    channel.name = "Router Channel";
    channel.provider = "Router Provider";
    channel.group = "Router Group";
    channel.radio = true;
    channel.encrypted = true;
    channel.enabled = true;
    snapshot.channels.push_back(channel);

    VdrEvent event;
    event.id = "router-event-1";
    event.channelId = "router-channel-1";
    event.title = "Router Event";
    event.subtitle = "Router Event Subtitle";
    event.description = "Router Event Description";
    event.startTime = "2026-06-04T20:00:00";
    event.endTime = "2026-06-04T21:00:00";
    event.durationSeconds = 3600;
    event.contentDescriptors.push_back("movie/drama");
    event.contentDescriptors.push_back("hd");
    event.parentalRating = 12;
    snapshot.events.push_back(event);

    VdrTimer timer;
    timer.id = "router-timer-1";
    timer.channelId = "router-channel-1";
    timer.eventId = "router-event-1";
    timer.title = "Router Timer";
    timer.subtitle = "Router Timer Subtitle";
    timer.startTime = "2026-06-04T20:00:00";
    timer.endTime = "2026-06-04T21:00:00";
    timer.priority = 50;
    timer.lifetime = 99;
    timer.enabled = true;
    timer.recording = true;
    snapshot.timers.push_back(timer);

    VdrRecording recording;
    recording.id = "router-recording-1";
    recording.backendId = "default";
    recording.title = "Router Recording";
    recording.path = "/srv/vdr/video/Router_Recording/2026-06-04.20.00.1-0.rec";
    recording.startTime = "2026-06-04T20:00:00";
    recording.durationSeconds = 3600;
    recording.sizeMb = 2048;
    recording.persons.add(Person::withProviderReference(
        ContentClassificationSource::Tvscraper,
        PersonRole::Actor,
        "Router Actor",
        "router-actor",
        "Router Character",
        95,
        "tvscraper:router-actor"));
    recording.persons.add(Person::withProviderReference(
        ContentClassificationSource::Tvscraper,
        PersonRole::Director,
        "Router Director",
        "router-director",
        "",
        90,
        "tvscraper:router-director"));
    snapshot.recordings.push_back(recording);

    return snapshot;
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
    EpgQueryService epgQueryService(vdrService);
    EpgSearchService epgSearchService;
    EpgSearchResultJsonSerializer epgSearchResultJsonSerializer;
    EpgController epgController(
        epgQueryService,
        epgSearchService,
        epgSearchResultJsonSerializer);

    PersonResolutionJsonSerializer personResolutionJsonSerializer;
    PersonSearchService personSearchService;
    PersonQueryResultJsonSerializer personQueryResultJsonSerializer;
    PersonController personController(
        personResolutionJsonSerializer,
        personSearchService,
        personQueryResultJsonSerializer);

    RecordingPersonSearchService recordingPersonSearchService;
    RecordingPersonSearchResultJsonSerializer recordingPersonSearchResultJsonSerializer;
    RecordingPersonSearchController recordingPersonSearchController(
        recordingPersonSearchService,
        recordingPersonSearchResultJsonSerializer);

    VdrOverviewService overviewService(
        vdrService);

    VdrOverviewJsonSerializer vdrJsonSerializer;

    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    snapshotCache.update(makeRouterSnapshot());

    VdrSnapshotReadService snapshotReadService(
        snapshotAccessService);

    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;

    VdrController vdrController(
        overviewService,
        vdrJsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);

    VdrRecordingQueryService vdrRecordingQueryService(vdrService);
    VdrRecordingQueryResultJsonSerializer vdrRecordingQueryJsonSerializer;
    VdrRecordingQueryController vdrRecordingQueryController(
        vdrRecordingQueryService,
        vdrRecordingQueryJsonSerializer);

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

    EpgSearchNativeFuzzyCapabilityRepository nativeFuzzyCapabilityRepository(
        db);
    EpgSearchNativeFuzzyCapabilityDetector nativeFuzzyCapabilityDetector;
    RouterNativeFuzzySearchTimerRuntime nativeFuzzyRuntime;
    EpgSearchNativeFuzzyOperatorRefreshService
        nativeFuzzyOperatorRefreshService(
            nativeFuzzyRuntime,
            nativeFuzzyRuntime,
            nativeFuzzyCapabilityRepository,
            nativeFuzzyCapabilityDetector,
            backendRegistryService);
    EpgSearchNativeFuzzyOperatorRefreshController
        nativeFuzzyOperatorRefreshController(
            nativeFuzzyOperatorRefreshService);

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
    snapshotChangeFeed.addEntry(SnapshotChangeFeedEntry(
        7,
        3,
        {"channels", "recordings"},
        "home-vdr"));
    snapshotChangeFeed.addEntry(SnapshotChangeFeedEntry(
        8,
        4,
        {"timers"},
        "ferienhaus-vdr"));
    SnapshotChangeFeedJsonSerializer snapshotChangeFeedJsonSerializer;
    SnapshotChangeFeedController snapshotChangeFeedController(
        snapshotChangeFeed,
        snapshotChangeFeedJsonSerializer);

    SseLiveTransport liveTransport;
    liveTransport.publish(LiveUpdateEvent(
        9,
        5,
        {"channels"},
        "home-vdr"));

    LiveTransportController liveTransportController(
        liveTransport);

    SearchTimerService searchTimerService;
    SearchTimerResultJsonSerializer searchTimerResultJsonSerializer;
    RouterSearchTimerDataSource searchTimerDataSource;
    SearchTimerController searchTimerController(
        searchTimerService,
        searchTimerResultJsonSerializer,
        searchTimerDataSource);

    VdrCapabilitySet capabilitySet =
        VdrCapabilitySet::snapshotReadOnly();

    CapabilityResolver capabilityResolver(capabilitySet);
    CapabilityReportBuilder capabilityReportBuilder;
    CapabilityReportService capabilityReportService(
        "router-backend",
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
    recordingActionBackendExecutorAdapterRegistry.registerAdapter(
        std::make_shared<RouterRecordingActionBackendExecutorAdapter>("router-backend"));
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
    vdrTimerActionExecutorAdapterRegistry.registerAdapter(
        std::make_shared<RouterVdrTimerActionExecutorAdapter>("router-backend"));

    ApiRouter router(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController,
        vdrRecordingQueryController,
        snapshotReadService,
        &epgController,
        &personController,
        &recordingPersonSearchController,
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
        &nativeFuzzyOperatorRefreshController);

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

    ApiResponse backendsResponse =
        router.handleGet("/api/backends");

    assert(backendsResponse.statusCode == 200);
    assert(backendsResponse.contentType == "application/json");
    assert(backendsResponse.body.find("\"backends\":[")
           != std::string::npos);
    assert(backendsResponse.body.find("\"backendId\":\"default\"")
           != std::string::npos);

    ApiResponse defaultBackendResponse =
        router.handleGet("/api/backends/default");

    assert(defaultBackendResponse.statusCode == 200);
    assert(defaultBackendResponse.contentType == "application/json");
    assert(defaultBackendResponse.body.find("\"backendId\":\"default\"")
           != std::string::npos);

    ApiResponse backendStatusResponse =
        router.handleGet("/api/backends/default/status");

    assert(backendStatusResponse.statusCode == 200);
    assert(backendStatusResponse.contentType == "application/json");
    assert(backendStatusResponse.body.find("\"mode\":\"router-snapshot\"")
           != std::string::npos);
    assert(backendStatusResponse.body.find("\"state\":\"cached\"")
           != std::string::npos);

    ApiResponse backendHealthResponse =
        router.handleGet("/api/backends/default/health");

    assert(backendHealthResponse.statusCode == 200);
    assert(backendHealthResponse.contentType == "application/json");
    assert(backendHealthResponse.body.find("\"backendId\":\"default\"")
           != std::string::npos);
    assert(backendHealthResponse.body.find("\"snapshotAvailable\":true")
           != std::string::npos);
    assert(backendHealthResponse.body.find("\"channelCount\":1")
           != std::string::npos);

    ApiResponse backendSnapshotResponse =
        router.handleGet("/api/backends/default/snapshot");

    assert(backendSnapshotResponse.statusCode == 200);
    assert(backendSnapshotResponse.contentType == "application/json");
    assert(backendSnapshotResponse.body.find("\"backendId\":\"default\"")
           != std::string::npos);
    assert(backendSnapshotResponse.body.find("\"snapshotAvailable\":true")
           != std::string::npos);
    assert(backendSnapshotResponse.body.find("\"recordingCount\":1")
           != std::string::npos);

    ApiResponse missingBackendStatusResponse =
        router.handleGet("/api/backends/ferienhaus/status");

    assert(missingBackendStatusResponse.statusCode == 200);
    assert(missingBackendStatusResponse.contentType == "application/json");
    assert(missingBackendStatusResponse.body.find("\"enabled\":false")
           != std::string::npos);

    ApiResponse missingBackendHealthResponse =
        router.handleGet("/api/backends/ferienhaus/health");

    assert(missingBackendHealthResponse.statusCode == 200);
    assert(missingBackendHealthResponse.contentType == "application/json");
    assert(missingBackendHealthResponse.body.find("\"backendId\":\"ferienhaus\"")
           != std::string::npos);
    assert(missingBackendHealthResponse.body.find("\"snapshotAvailable\":false")
           != std::string::npos);

    ApiResponse missingBackendSnapshotResponse =
        router.handleGet("/api/backends/ferienhaus/snapshot");

    assert(missingBackendSnapshotResponse.statusCode == 200);
    assert(missingBackendSnapshotResponse.contentType == "application/json");
    assert(missingBackendSnapshotResponse.body.find("\"backendId\":\"ferienhaus\"")
           != std::string::npos);
    assert(missingBackendSnapshotResponse.body.find("\"snapshotAvailable\":false")
           != std::string::npos);

    ApiResponse vdrAliasResponse =
        router.handleGet("/api/vdr");

    assert(vdrAliasResponse.statusCode == 200);
    assert(vdrAliasResponse.contentType == "application/json");
    assert(vdrAliasResponse.body.find("\"status\"")
           != std::string::npos);

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

    ApiResponse epgNowNextResponse =
        router.handleGet("/api/epg/now-next?channelId=router-channel&from=123");

    assert(epgNowNextResponse.statusCode == 200);
    assert(epgNowNextResponse.contentType == "application/json");
    assert(epgNowNextResponse.body.find("events")
           != std::string::npos);

    ApiResponse epgTimeWindowResponse =
        router.handleGet("/api/epg/time-window?channelId=router-channel&from=456&timespan=3600");

    assert(epgTimeWindowResponse.statusCode == 200);
    assert(epgTimeWindowResponse.contentType == "application/json");
    assert(epgTimeWindowResponse.body.find("events")
           != std::string::npos);

    ApiResponse epgChannelWindowResponse =
        router.handleGet("/api/epg/channel-window?channelId=router-channel&from=789&timespan=1800&limit=10");

    assert(epgChannelWindowResponse.statusCode == 200);
    assert(epgChannelWindowResponse.contentType == "application/json");
    assert(epgChannelWindowResponse.body.find("events")
           != std::string::npos);

    ApiResponse epgSearchResponse =
        router.handleGet("/api/epg/search?query=&backend=living-room&channelId=mock-channel-1&from=123&timespan=3600&limit=10&offset=0&sort=title&order=asc");

    assert(epgSearchResponse.statusCode == 200);
    assert(epgSearchResponse.contentType == "application/json");
    assert(epgSearchResponse.body.find("\"matches\":[")
           != std::string::npos);
    assert(epgSearchResponse.body.find("\"id\":\"mock-event-1\"")
           != std::string::npos);
    assert(epgSearchResponse.body.find("\"channelId\":\"mock-channel-1\"")
           != std::string::npos);
    assert(epgSearchResponse.body.find("\"title\":\"Tagesschau\"")
           != std::string::npos);

    ApiResponse epgSearchCaseInsensitiveResponse =
        router.handleGet("/api/epg/search?query=TATORT&channelId=mock-channel-1&from=123&timespan=3600&limit=10&offset=0&sort=title&order=asc");

    assert(epgSearchCaseInsensitiveResponse.statusCode == 200);
    assert(epgSearchCaseInsensitiveResponse.contentType == "application/json");
    assert(epgSearchCaseInsensitiveResponse.body.find("\"matches\":[")
           != std::string::npos);
    assert(epgSearchCaseInsensitiveResponse.body.find("\"id\":\"mock-event-2\"")
           != std::string::npos);
    assert(epgSearchCaseInsensitiveResponse.body.find("\"title\":\"Tatort\"")
           != std::string::npos);
    assert(epgSearchCaseInsensitiveResponse.body.find("\"id\":\"mock-event-1\"")
           == std::string::npos);

    ApiResponse epgSearchExactModeResponse =
        router.handleGet("/api/epg/search?query=Tatort&mode=exact&channelId=mock-channel-1&from=123&timespan=3600&limit=10&offset=0&sort=title&order=asc");

    assert(epgSearchExactModeResponse.statusCode == 200);
    assert(epgSearchExactModeResponse.contentType == "application/json");
    assert(epgSearchExactModeResponse.body.find("\"id\":\"mock-event-2\"")
           != std::string::npos);
    assert(epgSearchExactModeResponse.body.find("\"id\":\"mock-event-1\"")
           == std::string::npos);

    ApiResponse epgSearchInvalidModeResponse =
        router.handleGet("/api/epg/search?query=Tatort&mode=sideways&channelId=mock-channel-1&from=123&timespan=3600");

    assert(epgSearchInvalidModeResponse.statusCode == 400);
    assert(epgSearchInvalidModeResponse.contentType == "application/json");
    assert(epgSearchInvalidModeResponse.body.find("invalid search mode")
           != std::string::npos);

    ApiResponse epgSearchRegexModeResponse =
        router.handleGet("/api/epg/search?query=Tat.*&mode=regex&channelId=mock-channel-1&from=123&timespan=3600&limit=10&offset=0&sort=title&order=asc");

    assert(epgSearchRegexModeResponse.statusCode == 200);
    assert(epgSearchRegexModeResponse.contentType == "application/json");
    assert(epgSearchRegexModeResponse.body.find("\"id\":\"mock-event-2\"")
           != std::string::npos);
    assert(epgSearchRegexModeResponse.body.find("\"id\":\"mock-event-1\"")
           == std::string::npos);

    ApiResponse epgSearchInvalidRegexResponse =
        router.handleGet("/api/epg/search?query=[&mode=regex&channelId=mock-channel-1&from=123&timespan=3600");

    assert(epgSearchInvalidRegexResponse.statusCode == 400);
    assert(epgSearchInvalidRegexResponse.contentType == "application/json");
    assert(epgSearchInvalidRegexResponse.body.find("invalid regex pattern")
           != std::string::npos);

    ApiResponse epgSearchFuzzyModeResponse =
        router.handleGet("/api/epg/search?query=Tatorr&mode=fuzzy&tolerance=1&channelId=mock-channel-1&from=123&timespan=3600&limit=10&offset=0&sort=title&order=asc");

    assert(epgSearchFuzzyModeResponse.statusCode == 200);
    assert(epgSearchFuzzyModeResponse.contentType == "application/json");
    assert(epgSearchFuzzyModeResponse.body.find("\"id\":\"mock-event-2\"")
           != std::string::npos);
    assert(epgSearchFuzzyModeResponse.body.find("\"id\":\"mock-event-1\"")
           == std::string::npos);

    ApiResponse epgSearchFuzzyToleranceZeroResponse =
        router.handleGet("/api/epg/search?query=Tatorr&mode=fuzzy&tolerance=0&channelId=mock-channel-1&from=123&timespan=3600&limit=10&offset=0&sort=title&order=asc");

    assert(epgSearchFuzzyToleranceZeroResponse.statusCode == 200);
    assert(epgSearchFuzzyToleranceZeroResponse.contentType == "application/json");
    assert(epgSearchFuzzyToleranceZeroResponse.body.find("\"id\":\"mock-event-2\"")
           == std::string::npos);

    ApiResponse epgSearchInvalidFuzzyToleranceResponse =
        router.handleGet("/api/epg/search?query=Tatort&mode=fuzzy&tolerance=-1&channelId=mock-channel-1&from=123&timespan=3600");

    assert(epgSearchInvalidFuzzyToleranceResponse.statusCode == 400);
    assert(epgSearchInvalidFuzzyToleranceResponse.contentType == "application/json");
    assert(epgSearchInvalidFuzzyToleranceResponse.body.find("invalid fuzzy tolerance")
           != std::string::npos);

    ApiResponse epgSearchInvalidFuzzyToleranceTextResponse =
        router.handleGet("/api/epg/search?query=Tatort&mode=fuzzy&tolerance=abc&channelId=mock-channel-1&from=123&timespan=3600");

    assert(epgSearchInvalidFuzzyToleranceTextResponse.statusCode == 400);
    assert(epgSearchInvalidFuzzyToleranceTextResponse.contentType == "application/json");
    assert(epgSearchInvalidFuzzyToleranceTextResponse.body.find("invalid fuzzy tolerance")
           != std::string::npos);

    ApiResponse epgSearchInvalidTimespanResponse =
        router.handleGet("/api/epg/search?query=&channelId=mock-channel-1&from=123&timespan=0");

    assert(epgSearchInvalidTimespanResponse.statusCode == 400);
    assert(epgSearchInvalidTimespanResponse.contentType == "application/json");
    assert(epgSearchInvalidTimespanResponse.body.find("timespan must be greater than zero")
           != std::string::npos);

    ApiResponse epgSearchInvalidLimitResponse =
        router.handleGet("/api/epg/search?query=&channelId=mock-channel-1&from=123&timespan=3600&limit=-1");

    assert(epgSearchInvalidLimitResponse.statusCode == 400);
    assert(epgSearchInvalidLimitResponse.contentType == "application/json");
    assert(epgSearchInvalidLimitResponse.body.find("limit must not be negative")
           != std::string::npos);

    ApiResponse epgSearchInvalidOffsetResponse =
        router.handleGet("/api/epg/search?query=&channelId=mock-channel-1&from=123&timespan=3600&offset=-1");

    assert(epgSearchInvalidOffsetResponse.statusCode == 400);
    assert(epgSearchInvalidOffsetResponse.contentType == "application/json");
    assert(epgSearchInvalidOffsetResponse.body.find("offset must not be negative")
           != std::string::npos);

    ApiResponse epgSearchInvalidSortResponse =
        router.handleGet("/api/epg/search?query=&channelId=mock-channel-1&from=123&timespan=3600&sort=unknown");

    assert(epgSearchInvalidSortResponse.statusCode == 400);
    assert(epgSearchInvalidSortResponse.contentType == "application/json");
    assert(epgSearchInvalidSortResponse.body.find("invalid sort field")
           != std::string::npos);

    ApiResponse epgSearchInvalidOrderResponse =
        router.handleGet("/api/epg/search?query=&channelId=mock-channel-1&from=123&timespan=3600&order=sideways");

    assert(epgSearchInvalidOrderResponse.statusCode == 400);
    assert(epgSearchInvalidOrderResponse.contentType == "application/json");
    assert(epgSearchInvalidOrderResponse.body.find("invalid sort order")
           != std::string::npos);

    ApiResponse recordingPersonSearchResponse =
        router.handleGet("/api/recordings/persons/search?role=actor&limit=10&offset=0");

    assert(recordingPersonSearchResponse.statusCode == 200);
    assert(recordingPersonSearchResponse.contentType == "application/json");
    assert(recordingPersonSearchResponse.body.find("\"totalCount\":1")
           != std::string::npos);
    assert(recordingPersonSearchResponse.body.find("\"normalizedName\":\"router-actor\"")
           != std::string::npos);
    assert(recordingPersonSearchResponse.body.find("\"title\":\"Router Recording\"")
           != std::string::npos);

    ApiResponse vdrRecordingPersonSearchResponse =
        router.handleGet("/api/vdr/recordings/persons/search?normalizedName=router-director&backend=default");

    assert(vdrRecordingPersonSearchResponse.statusCode == 200);
    assert(vdrRecordingPersonSearchResponse.contentType == "application/json");
    assert(vdrRecordingPersonSearchResponse.body.find("\"totalCount\":1")
           != std::string::npos);
    assert(vdrRecordingPersonSearchResponse.body.find("\"role\":\"director\"")
           != std::string::npos);
    assert(vdrRecordingPersonSearchResponse.body.find("\"backendId\":\"default\"")
           != std::string::npos);

    ApiRouter routerWithoutEpg(
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

    ApiResponse unavailableEpgResponse =
        routerWithoutEpg.handleGet("/api/epg/now-next");

    assert(unavailableEpgResponse.statusCode == 503);
    assert(unavailableEpgResponse.contentType == "application/json");
    assert(unavailableEpgResponse.body.find("epg backend unavailable")
           != std::string::npos);


    ApiResponse unavailableNativeFuzzyRefreshResponse =
        routerWithoutEpg.handlePost(
            "/api/epgsearch/native-fuzzy/refresh",
            "{}");

    assert(unavailableNativeFuzzyRefreshResponse.statusCode == 503);
    assert(unavailableNativeFuzzyRefreshResponse.contentType == "application/json");
    assert(unavailableNativeFuzzyRefreshResponse.body.find(
               "native fuzzy operator refresh unavailable")
           != std::string::npos);

    ApiResponse vdrStatusResponse =
        router.handleGet("/api/vdr/status");

    assert(vdrStatusResponse.statusCode == 200);
    assert(vdrStatusResponse.contentType == "application/json");
    assert(vdrStatusResponse.body.find("\"mode\":\"router-snapshot\"")
           != std::string::npos);
    assert(vdrStatusResponse.body.find("\"host\":\"router-host\"")
           != std::string::npos);
    assert(vdrStatusResponse.body.find("\"port\":8002")
           != std::string::npos);

    ApiResponse vdrCapabilitiesResponse =
        router.handleGet("/api/vdr/capabilities");

    assert(vdrCapabilitiesResponse.statusCode == 200);
    assert(vdrCapabilitiesResponse.contentType == "application/json");
    assert(vdrCapabilitiesResponse.body.find("\"backendId\":\"router-backend\"")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"capabilities\":[")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"capability\":\"recordings.read\"")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"availability\":\"available\"")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"availableNow\":true")
           != std::string::npos);

    ApiResponse vdrSnapshotResponse =
        router.handleGet("/api/vdr/snapshot");

    assert(vdrSnapshotResponse.statusCode == 200);
    assert(vdrSnapshotResponse.contentType == "application/json");
    assert(vdrSnapshotResponse.body.find("\"snapshotAvailable\":true")
           != std::string::npos);
    assert(vdrSnapshotResponse.body.find("\"channelCount\":1")
           != std::string::npos);
    assert(vdrSnapshotResponse.body.find("\"eventCount\":1")
           != std::string::npos);
    assert(vdrSnapshotResponse.body.find("\"timerCount\":1")
           != std::string::npos);
    assert(vdrSnapshotResponse.body.find("\"recordingCount\":1")
           != std::string::npos);

    ApiResponse vdrSnapshotsResponse =
        router.handleGet("/api/vdr/snapshots");

    assert(vdrSnapshotsResponse.statusCode == 200);
    assert(vdrSnapshotsResponse.contentType == "application/json");
    assert(vdrSnapshotsResponse.body.find("\"snapshots\":[")
           != std::string::npos);
    assert(vdrSnapshotsResponse.body.find("\"backendId\":\"default\"")
           != std::string::npos);
    assert(vdrSnapshotsResponse.body.find("\"snapshotAvailable\":true")
           != std::string::npos);
    assert(vdrSnapshotsResponse.body.find("\"channelCount\":1")
           != std::string::npos);
    assert(vdrSnapshotsResponse.body.find("\"eventCount\":1")
           != std::string::npos);
    assert(vdrSnapshotsResponse.body.find("\"timerCount\":1")
           != std::string::npos);
    assert(vdrSnapshotsResponse.body.find("\"recordingCount\":1")
           != std::string::npos);

    ApiResponse vdrHealthResponse =
        router.handleGet("/api/vdr/health");

    assert(vdrHealthResponse.statusCode == 200);
    assert(vdrHealthResponse.contentType == "application/json");
    assert(vdrHealthResponse.body.find("\"snapshotAvailable\":true")
           != std::string::npos);
    assert(vdrHealthResponse.body.find("\"state\":\"cached\"")
           != std::string::npos);
    assert(vdrHealthResponse.body.find("\"mode\":\"router-snapshot\"")
           != std::string::npos);
    assert(vdrHealthResponse.body.find("\"channelCount\":1")
           != std::string::npos);
    assert(vdrHealthResponse.body.find("\"eventCount\":1")
           != std::string::npos);
    assert(vdrHealthResponse.body.find("\"timerCount\":1")
           != std::string::npos);
    assert(vdrHealthResponse.body.find("\"recordingCount\":1")
           != std::string::npos);

    ApiResponse searchTimersResponse =
        router.handleGet("/api/searchtimers?limit=25&offset=5");

    assert(searchTimersResponse.statusCode == 200);
    assert(searchTimersResponse.contentType == "application/json");
    assert(searchTimersResponse.body.find("\"searchtimers\":[]")
           != std::string::npos);
    assert(searchTimersResponse.body.find("\"limit\":25")
           != std::string::npos);
    assert(searchTimersResponse.body.find("\"offset\":5")
           != std::string::npos);

    ApiResponse searchTimerPreviewResponse =
        router.handleGet("/api/searchtimers/preview?query=Router&backend=default&limit=10&offset=0");

    assert(searchTimerPreviewResponse.statusCode == 200);
    assert(searchTimerPreviewResponse.contentType == "application/json");
    assert(searchTimerPreviewResponse.body.find("\"searchTimer\":{")
           != std::string::npos);
    assert(searchTimerPreviewResponse.body.find("\"backendId\":\"default\"")
           != std::string::npos);
    assert(searchTimerPreviewResponse.body.find("\"backendNativeId\":\"preview\"")
           != std::string::npos);
    assert(searchTimerPreviewResponse.body.find("\"query\":\"Router\"")
           != std::string::npos);
    assert(searchTimerPreviewResponse.body.find("\"preview\":{")
           != std::string::npos);
    assert(searchTimerPreviewResponse.body.find("\"totalCount\":1")
           != std::string::npos);
    assert(searchTimerPreviewResponse.body.find("\"id\":\"router-event-1\"")
           != std::string::npos);

    ApiResponse vdrSearchTimerPreviewResponse =
        router.handleGet("/api/vdr/searchtimers/preview?text=Router&limit=5");

    assert(vdrSearchTimerPreviewResponse.statusCode == 200);
    assert(vdrSearchTimerPreviewResponse.contentType == "application/json");
    assert(vdrSearchTimerPreviewResponse.body.find("\"query\":\"Router\"")
           != std::string::npos);
    assert(vdrSearchTimerPreviewResponse.body.find("\"returnedCount\":1")
           != std::string::npos);

    ApiResponse vdrSearchTimersResponse =
        router.handleGet("/api/vdr/searchtimers?backend=ferienhaus&state=inactive");

    assert(vdrSearchTimersResponse.statusCode == 200);
    assert(vdrSearchTimersResponse.contentType == "application/json");
    assert(vdrSearchTimersResponse.body.find("\"backendId\":\"ferienhaus\"")
           != std::string::npos);
    assert(vdrSearchTimersResponse.body.find("\"state\":\"inactive\"")
           != std::string::npos);


    ApiResponse searchTimerPlanResponse =
        router.handlePost(
            "/api/searchtimers/plan",
            "{"
            "\"operation\":\"create\","
            "\"backendId\":\"default\","
            "\"name\":\"Router Plan SearchTimer\","
            "\"query\":\"Router\","
            "\"active\":true"
            "}");

    assert(searchTimerPlanResponse.statusCode == 200);
    assert(searchTimerPlanResponse.contentType == "application/json");
    assert(searchTimerPlanResponse.body.find("\"valid\":true")
           != std::string::npos);
    assert(searchTimerPlanResponse.body.find("\"operation\":\"create\"")
           != std::string::npos);
    assert(searchTimerPlanResponse.body.find("\"primaryStep\":\"create\"")
           != std::string::npos);
    assert(searchTimerPlanResponse.body.find("\"followUpStep\":\"readback\"")
           != std::string::npos);
    assert(searchTimerPlanResponse.body.find("\"requiresBackendReadback\":true")
           != std::string::npos);

    ApiResponse vdrSearchTimerPlanResponse =
        router.handlePost(
            "/api/vdr/searchtimers/plan",
            "{"
            "\"operation\":\"delete\","
            "\"backendId\":\"default\","
            "\"backendNativeId\":\"searchtimer-42\""
            "}");

    assert(vdrSearchTimerPlanResponse.statusCode == 200);
    assert(vdrSearchTimerPlanResponse.contentType == "application/json");
    assert(vdrSearchTimerPlanResponse.body.find("\"operation\":\"delete\"")
           != std::string::npos);
    assert(vdrSearchTimerPlanResponse.body.find("\"primaryStep\":\"delete\"")
           != std::string::npos);
    assert(vdrSearchTimerPlanResponse.body.find("\"followUpStep\":\"none\"")
           != std::string::npos);
    assert(vdrSearchTimerPlanResponse.body.find("\"requiresExplicitOperatorConfirmation\":true")
           != std::string::npos);

    ApiResponse searchTimerExecuteBlockedResponse =
        router.handlePost(
            "/api/searchtimers/execute",
            "{"
            "\"operation\":\"create\","
            "\"backendId\":\"default\","
            "\"name\":\"Router Execute SearchTimer\","
            "\"query\":\"Router\""
            "}");

    assert(searchTimerExecuteBlockedResponse.statusCode == 200);
    assert(searchTimerExecuteBlockedResponse.contentType == "application/json");
    assert(searchTimerExecuteBlockedResponse.body.find("\"success\":false")
           != std::string::npos);
    assert(searchTimerExecuteBlockedResponse.body.find("\"blocked\":true")
           != std::string::npos);
    assert(searchTimerExecuteBlockedResponse.body.find("\"executed\":false")
           != std::string::npos);
    assert(searchTimerExecuteBlockedResponse.body.find("explicit operator confirmation is required")
           != std::string::npos);

    ApiResponse vdrSearchTimerExecuteAcceptedResponse =
        router.handlePost(
            "/api/vdr/searchtimers/execute",
            "{"
            "\"operation\":\"delete\","
            "\"backendId\":\"default\","
            "\"backendNativeId\":\"searchtimer-42\","
            "\"explicitOperatorConfirmation\":true"
            "}");

    assert(vdrSearchTimerExecuteAcceptedResponse.statusCode == 200);
    assert(vdrSearchTimerExecuteAcceptedResponse.contentType == "application/json");
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"success\":true")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"blocked\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executed\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"operation\":\"delete\"")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"primaryStep\":\"delete\"")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"commandRequestMapped\":true")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"realExecutionEnabled\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"realExecutionPolicyAllowed\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executorOptInProvided\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executorInjected\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executorInvocationGuardPassed\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executorInvocationAttempted\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executorInvocationKillSwitchOpen\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executorInvocationKillSwitchPassed\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executorResultMapped\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executorResultSuccessful\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"dispatchStage\":\"command-request-mapped\"")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("\"executionMode\":\"prepare\"")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("delete command request accepted by dispatch skeleton")
           != std::string::npos);
    assert(vdrSearchTimerExecuteAcceptedResponse.body.find("backend command dispatch is not enabled in this skeleton")
           != std::string::npos);

    ApiResponse vdrSearchTimerExecuteOptInResponse =
        router.handlePost(
            "/api/vdr/searchtimers/execute",
            "{"
            "\"operation\":\"delete\","
            "\"backendId\":\"default\","
            "\"backendNativeId\":\"searchtimer-42\","
            "\"executionMode\":\"execute\","
            "\"explicitOperatorConfirmation\":true,"
            "\"executorOptInEnabled\":true"
            "}");

    assert(vdrSearchTimerExecuteOptInResponse.statusCode == 200);
    assert(vdrSearchTimerExecuteOptInResponse.contentType == "application/json");
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"success\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"blocked\":true")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executed\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"commandRequestMapped\":true")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"realExecutionEnabled\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"realExecutionPolicyAllowed\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executorOptInProvided\":true")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executorInjected\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executorInvocationGuardPassed\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executorInvocationAttempted\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executorInvocationKillSwitchOpen\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executorInvocationKillSwitchPassed\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executorResultMapped\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executorResultSuccessful\":false")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"dispatchStage\":\"real-executor-injection-required\"")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("\"executionMode\":\"execute\"")
           != std::string::npos);
    assert(vdrSearchTimerExecuteOptInResponse.body.find("real execution mode requires an injected command executor")
           != std::string::npos);

    ApiResponse unavailableSearchTimersResponse =
        routerWithoutEpg.handleGet("/api/searchtimers");

    assert(unavailableSearchTimersResponse.statusCode == 503);
    assert(unavailableSearchTimersResponse.contentType == "application/json");
    assert(unavailableSearchTimersResponse.body.find("searchtimer backend unavailable")
           != std::string::npos);

    ApiResponse vdrChangesResponse =
        router.handleGet("/api/vdr/changes");

    assert(vdrChangesResponse.statusCode == 200);
    assert(vdrChangesResponse.contentType == "application/json");
    assert(vdrChangesResponse.body.find("\"latestSequenceNumber\":8")
           != std::string::npos);
    assert(vdrChangesResponse.body.find("\"latestSnapshotGeneration\":4")
           != std::string::npos);
    assert(vdrChangesResponse.body.find("\"backendId\":\"home-vdr\"")
           != std::string::npos);
    assert(vdrChangesResponse.body.find("\"backendId\":\"ferienhaus-vdr\"")
           != std::string::npos);
    assert(vdrChangesResponse.body.find("\"channels\"")
           != std::string::npos);
    assert(vdrChangesResponse.body.find("\"recordings\"")
           != std::string::npos);
    assert(vdrChangesResponse.body.find("\"timers\"")
           != std::string::npos);

    ApiResponse vdrLiveResponse =
        router.handleGet("/api/vdr/live");

    assert(vdrLiveResponse.statusCode == 200);
    assert(vdrLiveResponse.contentType == "text/event-stream");
    assert(vdrLiveResponse.body.find("event: update\n")
           != std::string::npos);
    assert(vdrLiveResponse.body.find("id: 9\n")
           != std::string::npos);
    assert(vdrLiveResponse.body.find("\"backendId\":\"home-vdr\"")
           != std::string::npos);
    assert(vdrLiveResponse.body.find("\"changedDomains\":[\"channels\"]")
           != std::string::npos);

    ApiResponse personQueryResponse =
        router.handleGet("/api/persons?role=actor&limit=10&offset=0");

    assert(personQueryResponse.statusCode == 200);
    assert(personQueryResponse.contentType == "application/json");
    assert(personQueryResponse.body.find("\"totalCount\":0")
           != std::string::npos);
    assert(personQueryResponse.body.find("\"returnedCount\":0")
           != std::string::npos);
    assert(personQueryResponse.body.find("\"persons\":[]")
           != std::string::npos);

    ApiResponse vdrPersonQueryResponse =
        router.handleGet("/api/vdr/persons?source=tmdb");

    assert(vdrPersonQueryResponse.statusCode == 200);
    assert(vdrPersonQueryResponse.contentType == "application/json");
    assert(vdrPersonQueryResponse.body.find("\"persons\":[]")
           != std::string::npos);

    ApiResponse invalidPersonQueryResponse =
        router.handleGet("/api/persons?role=hero");

    assert(invalidPersonQueryResponse.statusCode == 400);
    assert(invalidPersonQueryResponse.body.find("invalid person role")
           != std::string::npos);

    ApiResponse unavailablePersonQueryResponse =
        routerWithoutEpg.handleGet("/api/persons");

    assert(unavailablePersonQueryResponse.statusCode == 503);
    assert(unavailablePersonQueryResponse.body.find("person search unavailable")
           != std::string::npos);

    ApiResponse vdrRecordingQueryResponse =
        router.handleGet("/api/vdr/recordings/query?title=tatort&limit=10&offset=0");

    assert(vdrRecordingQueryResponse.statusCode == 200);
    assert(vdrRecordingQueryResponse.contentType == "application/json");
    assert(vdrRecordingQueryResponse.body.find("\"totalCount\":1")
           != std::string::npos);
    assert(vdrRecordingQueryResponse.body.find("\"returnedCount\":1")
           != std::string::npos);
    assert(vdrRecordingQueryResponse.body.find("\"title\":\"Tatort\"")
           != std::string::npos);
    assert(vdrRecordingQueryResponse.body.find("\"title\":\"Tagesschau\"")
           == std::string::npos);

    ApiResponse vdrRecordingPathQueryResponse =
        router.handleGet("/api/vdr/recordings/query?path=Tagesschau&limit=10&offset=0");

    assert(vdrRecordingPathQueryResponse.statusCode == 200);
    assert(vdrRecordingPathQueryResponse.contentType == "application/json");
    assert(vdrRecordingPathQueryResponse.body.find("\"totalCount\":1")
           != std::string::npos);
    assert(vdrRecordingPathQueryResponse.body.find("\"returnedCount\":1")
           != std::string::npos);
    assert(vdrRecordingPathQueryResponse.body.find("\"title\":\"Tagesschau\"")
           != std::string::npos);
    assert(vdrRecordingPathQueryResponse.body.find("\"title\":\"Tatort\"")
           == std::string::npos);

    ApiResponse vdrRecordingSortQueryResponse =
        router.handleGet("/api/vdr/recordings/query?sort=title&order=desc");

    assert(vdrRecordingSortQueryResponse.statusCode == 200);
    assert(vdrRecordingSortQueryResponse.contentType == "application/json");
    assert(vdrRecordingSortQueryResponse.body.find("\"title\":\"Tatort\"") <
           vdrRecordingSortQueryResponse.body.find("\"title\":\"Tagesschau\""));

    ApiResponse vdrRecordingStartTimeSortQueryResponse =
        router.handleGet("/api/vdr/recordings/query?sort=startTime&order=desc");

    assert(vdrRecordingStartTimeSortQueryResponse.statusCode == 200);
    assert(vdrRecordingStartTimeSortQueryResponse.contentType == "application/json");
    assert(vdrRecordingStartTimeSortQueryResponse.body.find("\"title\":\"Tatort\"") <
           vdrRecordingStartTimeSortQueryResponse.body.find("\"title\":\"Tagesschau\""));

    ApiResponse vdrRecordingRangeQueryResponse =
        router.handleGet("/api/vdr/recordings/query?from=2026-06-01T20:00:00&to=2026-06-01T21:00:00");

    assert(vdrRecordingRangeQueryResponse.statusCode == 200);
    assert(vdrRecordingRangeQueryResponse.contentType == "application/json");
    assert(vdrRecordingRangeQueryResponse.body.find("\"totalCount\":2")
           != std::string::npos);

    ApiResponse vdrRecordingDurationQueryResponse =
        router.handleGet("/api/vdr/recordings/query?durationMin=900&durationMax=7200");

    assert(vdrRecordingDurationQueryResponse.statusCode == 200);
    assert(vdrRecordingDurationQueryResponse.contentType == "application/json");
    assert(vdrRecordingDurationQueryResponse.body.find("\"totalCount\":2")
           != std::string::npos);

    ApiResponse vdrRecordingsResponse =
        router.handleGet("/api/vdr/recordings");

    assert(vdrRecordingsResponse.statusCode == 200);
    assert(vdrRecordingsResponse.contentType == "application/json");
    assert(vdrRecordingsResponse.body.find("\"recordings\"")
           != std::string::npos);
    assert(vdrRecordingsResponse.body.find("\"id\":\"router-recording-1\"")
           != std::string::npos);
    assert(vdrRecordingsResponse.body.find("\"title\":\"Router Recording\"")
           != std::string::npos);
    assert(vdrRecordingsResponse.body.find("\"path\":\"/srv/vdr/video/Router_Recording/2026-06-04.20.00.1-0.rec\"")
           != std::string::npos);
    assert(vdrRecordingsResponse.body.find("\"startTime\":\"2026-06-04T20:00:00\"")
           != std::string::npos);
    assert(vdrRecordingsResponse.body.find("\"durationSeconds\":3600")
           != std::string::npos);
    assert(vdrRecordingsResponse.body.find("\"sizeMb\":2048")
           != std::string::npos);

    ApiResponse vdrTimersResponse =
        router.handleGet("/api/vdr/timers");

    assert(vdrTimersResponse.statusCode == 200);
    assert(vdrTimersResponse.contentType == "application/json");
    assert(vdrTimersResponse.body.find("\"timers\"")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"id\":\"router-timer-1\"")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"channelId\":\"router-channel-1\"")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"eventId\":\"router-event-1\"")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"title\":\"Router Timer\"")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"subtitle\":\"Router Timer Subtitle\"")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"startTime\":\"2026-06-04T20:00:00\"")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"endTime\":\"2026-06-04T21:00:00\"")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"priority\":50")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"lifetime\":99")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"enabled\":true")
           != std::string::npos);
    assert(vdrTimersResponse.body.find("\"recording\":true")
           != std::string::npos);

    ApiResponse vdrChannelsResponse =
        router.handleGet("/api/vdr/channels");

    assert(vdrChannelsResponse.statusCode == 200);
    assert(vdrChannelsResponse.contentType == "application/json");
    assert(vdrChannelsResponse.body.find("\"channels\"")
           != std::string::npos);
    assert(vdrChannelsResponse.body.find("\"id\":\"router-channel-1\"")
           != std::string::npos);
    assert(vdrChannelsResponse.body.find("\"number\":1")
           != std::string::npos);
    assert(vdrChannelsResponse.body.find("\"name\":\"Router Channel\"")
           != std::string::npos);
    assert(vdrChannelsResponse.body.find("\"provider\":\"Router Provider\"")
           != std::string::npos);
    assert(vdrChannelsResponse.body.find("\"group\":\"Router Group\"")
           != std::string::npos);
    assert(vdrChannelsResponse.body.find("\"radio\":true")
           != std::string::npos);
    assert(vdrChannelsResponse.body.find("\"encrypted\":true")
           != std::string::npos);
    assert(vdrChannelsResponse.body.find("\"enabled\":true")
           != std::string::npos);

    ApiResponse vdrEventsResponse =
        router.handleGet("/api/vdr/events");

    assert(vdrEventsResponse.statusCode == 200);
    assert(vdrEventsResponse.contentType == "application/json");
    assert(vdrEventsResponse.body.find("\"events\"")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"id\":\"router-event-1\"")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"channelId\":\"router-channel-1\"")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"title\":\"Router Event\"")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"subtitle\":\"Router Event Subtitle\"")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"description\":\"Router Event Description\"")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"startTime\":\"2026-06-04T20:00:00\"")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"endTime\":\"2026-06-04T21:00:00\"")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"durationSeconds\":3600")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"contentDescriptors\":[\"movie/drama\",\"hd\"]")
           != std::string::npos);
    assert(vdrEventsResponse.body.find("\"parentalRating\":12")
           != std::string::npos);


    const std::string executeBody =
        "{"
        "\"backendId\":\"router-backend\","
        "\"recordingId\":\"router-recording-1\","
        "\"action\":\"DELETE\","
        "\"dryRun\":true"
        "}";

    ApiResponse actionExecuteResponse =
        router.handlePost(
            "/api/recordings/actions/execute",
            executeBody);

    assert(actionExecuteResponse.statusCode == 200);
    assert(actionExecuteResponse.contentType == "application/json");
    assert(actionExecuteResponse.body.find("\"success\":false")
           != std::string::npos);
    assert(actionExecuteResponse.body.find("\"type\":\"DELETE\"")
           != std::string::npos);
    assert(actionExecuteResponse.body.find("\"backendId\":\"router-backend\"")
           != std::string::npos);
    assert(actionExecuteResponse.body.find("\"recordingId\":\"router-recording-1\"")
           != std::string::npos);
    assert(actionExecuteResponse.body.find("\"message\":\"router dry-run execution skipped\"")
           != std::string::npos);

    ApiResponse actionExecuteAliasResponse =
        router.handlePost(
            "/api/vdr/recordings/actions/execute",
            executeBody);

    assert(actionExecuteAliasResponse.statusCode == 200);
    assert(actionExecuteAliasResponse.contentType == "application/json");
    assert(actionExecuteAliasResponse.body.find("\"type\":\"DELETE\"")
           != std::string::npos);

    const std::string timerCreateBody =
        "{"
        "\"backendId\":\"router-backend\","
        "\"timerId\":\"router-timer-2\","
        "\"channelId\":\"router-channel-1\","
        "\"title\":\"Router Timer Create\""
        "}";

    ApiResponse timerCreateResponse =
        router.handlePost(
            "/api/vdr/timers/actions/create",
            timerCreateBody);

    assert(timerCreateResponse.statusCode == 200);
    assert(timerCreateResponse.contentType == "application/json");
    assert(timerCreateResponse.body.find("\"success\":true")
           != std::string::npos);
    assert(timerCreateResponse.body.find("\"type\":\"create\"")
           != std::string::npos);
    assert(timerCreateResponse.body.find("\"backendId\":\"router-backend\"")
           != std::string::npos);

    const std::string timerUpdateBody =
        "{"
        "\"backendId\":\"router-backend\","
        "\"timerId\":\"router-timer-1\","
        "\"title\":\"Router Timer Update\""
        "}";

    ApiResponse timerUpdateResponse =
        router.handlePost(
            "/api/vdr/timers/actions/update",
            timerUpdateBody);

    assert(timerUpdateResponse.statusCode == 200);
    assert(timerUpdateResponse.contentType == "application/json");
    assert(timerUpdateResponse.body.find("\"type\":\"update\"")
           != std::string::npos);

    const std::string timerDeleteBody =
        "{"
        "\"backendId\":\"router-backend\","
        "\"timerId\":\"router-timer-1\""
        "}";

    ApiResponse timerDeleteResponse =
        router.handlePost(
            "/api/vdr/timers/actions/delete",
            timerDeleteBody);

    assert(timerDeleteResponse.statusCode == 200);
    assert(timerDeleteResponse.contentType == "application/json");
    assert(timerDeleteResponse.body.find("\"type\":\"delete\"")
           != std::string::npos);

    ApiResponse timerMissingBackendResponse =
        router.handlePost(
            "/api/vdr/timers/actions/delete",
            "{\"backendId\":\"missing-backend\",\"timerId\":\"router-timer-1\"}");

    assert(timerMissingBackendResponse.statusCode == 200);
    assert(timerMissingBackendResponse.contentType == "application/json");
    assert(timerMissingBackendResponse.body.find("\"success\":false")
           != std::string::npos);
    assert(timerMissingBackendResponse.body.find("timer action executor adapter not found")
           != std::string::npos);


    ApiResponse nativeFuzzyRefreshResponse =
        router.handlePost(
            "/api/epgsearch/native-fuzzy/refresh",
            "{"
            "\"backendId\":\"default\","
            "\"probeQuery\":\"router native fuzzy probe\","
            "\"tolerance\":2"
            "}");

    assert(nativeFuzzyRefreshResponse.statusCode == 200);
    assert(nativeFuzzyRefreshResponse.contentType == "application/json");
    assert(nativeFuzzyRefreshResponse.body.find(
               "\"status\":\"refreshed-native-available\"")
           != std::string::npos);
    assert(nativeFuzzyRefreshResponse.body.find("\"backendId\":\"default\"")
           != std::string::npos);
    assert(nativeFuzzyRefreshResponse.body.find("\"cleanupSucceeded\":true")
           != std::string::npos);
    assert(nativeFuzzyRefreshResponse.body.find("\"nativeFuzzyAvailable\":true")
           != std::string::npos);
    assert(nativeFuzzyRefreshResponse.body.find(
               "\"backendCapabilitiesUpdated\":true")
           != std::string::npos);
    assert(nativeFuzzyRuntime.createCalls == 1);
    assert(nativeFuzzyRuntime.deleteCalls == 1);
    assert(nativeFuzzyRuntime.timers.empty());

    ApiResponse nativeFuzzyRefreshAliasResponse =
        router.handlePost(
            "/api/vdr/epgsearch/native-fuzzy/refresh",
            "{"
            "\"backend\":\"default\","
            "\"query\":\"router native fuzzy alias probe\","
            "\"tolerance\":2"
            "}");

    assert(nativeFuzzyRefreshAliasResponse.statusCode == 200);
    assert(nativeFuzzyRefreshAliasResponse.contentType == "application/json");
    assert(nativeFuzzyRefreshAliasResponse.body.find(
               "\"status\":\"refreshed-native-available\"")
           != std::string::npos);
    assert(nativeFuzzyRuntime.createCalls == 2);
    assert(nativeFuzzyRuntime.deleteCalls == 2);
    assert(nativeFuzzyRuntime.timers.empty());

    ApiResponse runtimeAliasResponse =
        router.handleGet("/api/runtime/diagnostics");

    assert(runtimeAliasResponse.statusCode == 200);
    assert(runtimeAliasResponse.contentType == "application/json");
    assert(runtimeAliasResponse.body.find("\"measurements\"")
           != std::string::npos);

    ApiResponse runtimeResponse =
        router.handleGet("/api/runtime");

    assert(runtimeResponse.statusCode == 200);
    assert(runtimeResponse.contentType == "application/json");
    assert(runtimeResponse.body.find("\"measurements\"")
           != std::string::npos);
    assert(runtimeResponse.body.find("\"component\":\"PollingService\"")
           != std::string::npos);
    assert(runtimeResponse.body.find("\"operation\":\"Poll cycle\"")
           != std::string::npos);
    assert(runtimeResponse.body.find("\"durationMs\":30")
           != std::string::npos);
    assert(runtimeResponse.body.find("\"durationMs\":10")
           != std::string::npos);

    ApiResponse runtimeSummaryAliasResponse =
        router.handleGet("/api/runtime/diagnostics/summary");

    assert(runtimeSummaryAliasResponse.statusCode == 200);
    assert(runtimeSummaryAliasResponse.contentType == "application/json");
    assert(runtimeSummaryAliasResponse.body.find("\"summaries\"")
           != std::string::npos);

    ApiResponse runtimeSummaryResponse =
        router.handleGet("/api/runtime/summary");

    assert(runtimeSummaryResponse.statusCode == 200);
    assert(runtimeSummaryResponse.contentType == "application/json");
    assert(runtimeSummaryResponse.body.find("\"summaries\"")
           != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"component\":\"PollingService\"")
           != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"operation\":\"Poll cycle\"")
           != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"count\":2")
           != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"minDurationMs\":10")
           != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"maxDurationMs\":30")
           != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"lastDurationMs\":10")
           != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"lastSizeBytes\":200")
           != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"lastItemCount\":12")
           != std::string::npos);

    ApiResponse missingResponse =
        router.handleGet("/api/unknown");

    assert(missingResponse.statusCode == 404);

    db.close();

    ApiResponse searchTimerValidationResponse =
        router.handlePost(
            "/api/searchtimers/validate",
            "{"
            "\"operation\":\"create\","
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Terra X Suche\","
            "\"query\":\"Terra X\""
            "}");

    assert(searchTimerValidationResponse.statusCode == 200);
    assert(searchTimerValidationResponse.contentType == "application/json");
    assert(searchTimerValidationResponse.body.find("\"valid\":true") != std::string::npos);
    assert(searchTimerValidationResponse.body.find("\"operation\":\"create\"") != std::string::npos);
    assert(searchTimerValidationResponse.body.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(searchTimerValidationResponse.body.find("\"errors\":[]") != std::string::npos);

    ApiResponse searchTimerValidationAliasResponse =
        router.handlePost(
            "/api/vdr/searchtimers/validate",
            "{"
            "\"operation\":\"update\","
            "\"backendId\":\"home-vdr\","
            "\"name\":\"Terra X Suche\","
            "\"query\":\"Terra X\""
            "}");

    assert(searchTimerValidationAliasResponse.statusCode == 200);
    assert(searchTimerValidationAliasResponse.contentType == "application/json");
    assert(searchTimerValidationAliasResponse.body.find("\"valid\":false") != std::string::npos);
    assert(searchTimerValidationAliasResponse.body.find("\"operation\":\"update\"") != std::string::npos);
    assert(searchTimerValidationAliasResponse.body.find("\"backendNativeId is required\"") != std::string::npos);

    std::cout
        << "test_api_router passed"
        << std::endl;

    return 0;
}
