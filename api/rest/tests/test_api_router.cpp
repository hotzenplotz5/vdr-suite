#include "ApiRouter.h"
#include "BackendRegistryService.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryController.h"
#include "BackendRegistry.h"
#include "DashboardController.h"
#include "JobsController.h"
#include "MetadataController.h"
#include "RecordingsController.h"
#include "RuntimeDiagnosticsController.h"
#include "SnapshotChangeFeedController.h"
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
#include "RuntimeMeasurement.h"
#include "SnapshotAccessService.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedJsonSerializer.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrSnapshotReadService.h"

#include <cassert>
#include <iostream>
#include <string>

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
    recording.title = "Router Recording";
    recording.path = "/srv/vdr/video/Router_Recording/2026-06-04.20.00.1-0.rec";
    recording.startTime = "2026-06-04T20:00:00";
    recording.durationSeconds = 3600;
    recording.sizeMb = 2048;
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
    snapshotChangeFeed.addEntry(SnapshotChangeFeedEntry(7, 3, {"channels", "recordings"}));
    SnapshotChangeFeedJsonSerializer snapshotChangeFeedJsonSerializer;
    SnapshotChangeFeedController snapshotChangeFeedController(
        snapshotChangeFeed,
        snapshotChangeFeedJsonSerializer);

    ApiRouter router(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController,
        backendRegistryController,
        runtimeDiagnosticsController,
        snapshotChangeFeedController);

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
    assert(vdrCapabilitiesResponse.body.find("\"snapshotRead\":true")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"statusRead\":true")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"healthRead\":true")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"recordingsRead\":true")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"timersRead\":true")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"channelsRead\":true")
           != std::string::npos);
    assert(vdrCapabilitiesResponse.body.find("\"eventsRead\":true")
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

    ApiResponse vdrChangesResponse =
        router.handleGet("/api/vdr/changes");

    assert(vdrChangesResponse.statusCode == 200);
    assert(vdrChangesResponse.contentType == "application/json");
    assert(vdrChangesResponse.body.find("\"latestSequenceNumber\":7")
           != std::string::npos);
    assert(vdrChangesResponse.body.find("\"channels\"")
           != std::string::npos);
    assert(vdrChangesResponse.body.find("\"recordings\"")
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

    std::cout
        << "test_api_router passed"
        << std::endl;

    return 0;
}
