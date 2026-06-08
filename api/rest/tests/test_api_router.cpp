#include "ApiRouter.h"
#include "DashboardController.h"
#include "JobsController.h"
#include "MetadataController.h"
#include "RecordingsController.h"
#include "RuntimeDiagnosticsController.h"
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

    ApiRouter router(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController,
        runtimeDiagnosticsController);

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
