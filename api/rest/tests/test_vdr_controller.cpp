#include "MockVdrAdapter.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
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

static VdrSnapshot makeControllerSnapshot()
{
    VdrSnapshot snapshot;

    snapshot.status.enabled = true;
    snapshot.status.mode = "snapshot-controller";
    snapshot.status.host = "snapshot-controller-host";
    snapshot.status.port = 1234;
    snapshot.status.state = "cached";

    VdrChannel channel;
    channel.id = "snapshot-channel-1";
    channel.number = 1;
    channel.name = "Snapshot Channel";
    channel.provider = "Snapshot Provider";
    channel.group = "Snapshot Group";
    channel.radio = true;
    channel.encrypted = true;
    channel.enabled = true;
    snapshot.channels.push_back(channel);

    VdrEvent event;
    event.id = "snapshot-event-1";
    event.channelId = "snapshot-channel-1";
    event.title = "Snapshot Event";
    event.subtitle = "Snapshot Event Subtitle";
    event.description = "Snapshot Event Description";
    event.startTime = "2026-06-04T20:00:00";
    event.endTime = "2026-06-04T21:00:00";
    event.durationSeconds = 3600;
    event.parentalRating = 0;
    snapshot.events.push_back(event);

    VdrTimer timer;
    timer.id = "snapshot-timer-1";
    timer.channelId = "snapshot-channel-1";
    timer.eventId = "snapshot-event-1";
    timer.title = "Snapshot Timer";
    timer.subtitle = "Snapshot Timer Subtitle";
    timer.startTime = "2026-06-04T20:00:00";
    timer.endTime = "2026-06-04T21:00:00";
    timer.priority = 50;
    timer.lifetime = 99;
    timer.enabled = true;
    timer.recording = true;
    snapshot.timers.push_back(timer);

    VdrRecording recording;
    recording.id = "snapshot-recording-1";
    recording.title = "Snapshot Recording";
    recording.path = "/srv/vdr/video/Snapshot_Recording/2026-06-04.20.00.1-0.rec";
    recording.startTime = "2026-06-04T20:00:00";
    recording.durationSeconds = 3600;
    recording.sizeMb = 2048;
    snapshot.recordings.push_back(recording);

    return snapshot;
}

static void assertJsonResponse(const ApiResponse& response)
{
    assert(response.statusCode == 200);

    assert(response.contentType
           == "application/json");
}

static void assertCommonOverviewResponse(const ApiResponse& response)
{
    assertJsonResponse(response);

    assert(response.body.find("\"status\"")
           != std::string::npos);

    assert(response.body.find("\"channels\"")
           != std::string::npos);

    assert(response.body.find("\"events\"")
           != std::string::npos);

    assert(response.body.find("\"timers\"")
           != std::string::npos);

    assert(response.body.find("\"recordings\"")
           != std::string::npos);
}

static VdrController makeLiveController(
    VdrOverviewService& overviewService,
    VdrOverviewJsonSerializer& jsonSerializer,
    VdrSnapshotReadService& snapshotReadService,
    VdrSnapshotReadJsonSerializer& snapshotReadJsonSerializer)
{
    return VdrController(
        overviewService,
        jsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);
}

static void test_vdr_controller_returns_live_overview()
{
    MockVdrAdapter adapter;

    VdrService vdrService(adapter);

    VdrOverviewService overviewService(
        vdrService);

    VdrOverviewJsonSerializer jsonSerializer;

    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);
    VdrSnapshotReadService snapshotReadService(accessService);
    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;

    VdrController controller = makeLiveController(
        overviewService,
        jsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);

    ApiResponse response =
        controller.getOverview();

    assertCommonOverviewResponse(response);

    assert(response.body.find("\"totalChannels\":3")
           != std::string::npos);

    assert(response.body.find("\"totalEvents\":2")
           != std::string::npos);

    assert(response.body.find("\"totalRecordings\":2")
           != std::string::npos);
}

static void test_vdr_controller_returns_snapshot_backed_overview()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshot snapshot = makeControllerSnapshot();
    cache.update(snapshot);

    VdrOverviewService overviewService(accessService);
    VdrOverviewJsonSerializer jsonSerializer;
    VdrSnapshotReadService snapshotReadService(accessService);
    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;

    VdrController controller = makeLiveController(
        overviewService,
        jsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);

    ApiResponse response =
        controller.getOverview();

    assertCommonOverviewResponse(response);

    assert(response.body.find("\"mode\":\"snapshot-controller\"")
           != std::string::npos);

    assert(response.body.find("\"host\":\"snapshot-controller-host\"")
           != std::string::npos);

    assert(response.body.find("\"state\":\"cached\"")
           != std::string::npos);

    assert(response.body.find("\"totalChannels\":1")
           != std::string::npos);

    assert(response.body.find("\"radioChannels\":1")
           != std::string::npos);

    assert(response.body.find("\"encryptedChannels\":1")
           != std::string::npos);

    assert(response.body.find("\"totalEvents\":1")
           != std::string::npos);

    assert(response.body.find("\"totalTimers\":1")
           != std::string::npos);

    assert(response.body.find("\"activeTimers\":1")
           != std::string::npos);

    assert(response.body.find("\"recordingTimers\":1")
           != std::string::npos);

    assert(response.body.find("\"hasNextTimer\":true")
           != std::string::npos);

    assert(response.body.find("\"id\":\"snapshot-timer-1\"")
           != std::string::npos);

    assert(response.body.find("\"totalRecordings\":1")
           != std::string::npos);

    assert(response.body.find("\"hasLatestRecording\":true")
           != std::string::npos);

    assert(response.body.find("\"title\":\"Snapshot Recording\"")
           != std::string::npos);
}

static void test_vdr_controller_returns_snapshot_read_status()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshot snapshot = makeControllerSnapshot();
    cache.update(snapshot);

    VdrOverviewService overviewService(accessService);
    VdrOverviewJsonSerializer jsonSerializer;
    VdrSnapshotReadService snapshotReadService(accessService);
    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;

    VdrController controller = makeLiveController(
        overviewService,
        jsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);

    ApiResponse response =
        controller.getStatus();

    assertJsonResponse(response);

    assert(response.body.find("\"enabled\":true")
           != std::string::npos);

    assert(response.body.find("\"mode\":\"snapshot-controller\"")
           != std::string::npos);

    assert(response.body.find("\"host\":\"snapshot-controller-host\"")
           != std::string::npos);

    assert(response.body.find("\"port\":1234")
           != std::string::npos);

    assert(response.body.find("\"state\":\"cached\"")
           != std::string::npos);
}

static void test_vdr_controller_returns_snapshot_read_domain_lists()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshot snapshot = makeControllerSnapshot();
    cache.update(snapshot);

    VdrOverviewService overviewService(accessService);
    VdrOverviewJsonSerializer jsonSerializer;
    VdrSnapshotReadService snapshotReadService(accessService);
    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;

    VdrController controller = makeLiveController(
        overviewService,
        jsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);

    ApiResponse recordingsResponse =
        controller.getRecordings();
    assertJsonResponse(recordingsResponse);
    assert(recordingsResponse.body == "{\"recordings\":[]}");

    ApiResponse timersResponse =
        controller.getTimers();
    assertJsonResponse(timersResponse);
    assert(timersResponse.body == "{\"timers\":[]}");

    ApiResponse channelsResponse =
        controller.getChannels();
    assertJsonResponse(channelsResponse);
    assert(channelsResponse.body.find("\"channels\"")
           != std::string::npos);
    assert(channelsResponse.body.find("\"id\":\"snapshot-channel-1\"")
           != std::string::npos);
    assert(channelsResponse.body.find("\"number\":1")
           != std::string::npos);
    assert(channelsResponse.body.find("\"name\":\"Snapshot Channel\"")
           != std::string::npos);
    assert(channelsResponse.body.find("\"provider\":\"Snapshot Provider\"")
           != std::string::npos);
    assert(channelsResponse.body.find("\"group\":\"Snapshot Group\"")
           != std::string::npos);
    assert(channelsResponse.body.find("\"radio\":true")
           != std::string::npos);
    assert(channelsResponse.body.find("\"encrypted\":true")
           != std::string::npos);
    assert(channelsResponse.body.find("\"enabled\":true")
           != std::string::npos);

    ApiResponse eventsResponse =
        controller.getEvents();
    assertJsonResponse(eventsResponse);
    assert(eventsResponse.body == "{\"events\":[]}");
}

int main()
{
    test_vdr_controller_returns_live_overview();
    test_vdr_controller_returns_snapshot_backed_overview();
    test_vdr_controller_returns_snapshot_read_status();
    test_vdr_controller_returns_snapshot_read_domain_lists();

    std::cout
        << "test_vdr_controller passed"
        << std::endl;

    return 0;
}
