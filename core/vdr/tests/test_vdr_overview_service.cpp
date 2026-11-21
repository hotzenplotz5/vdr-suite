#include "MockVdrAdapter.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrOverview.h"
#include "VdrOverviewService.h"
#include "VdrService.h"
#include "VdrSnapshot.h"

#include <cassert>

static VdrSnapshot makeSnapshotOverviewTestSnapshot()
{
    VdrSnapshot snapshot;

    snapshot.status.enabled = true;
    snapshot.status.mode = "snapshot";
    snapshot.status.host = "snapshot-host";
    snapshot.status.port = 1234;
    snapshot.status.state = "cached";

    VdrChannel tvChannel;
    tvChannel.id = "snapshot-channel-1";
    tvChannel.number = 1;
    tvChannel.name = "Snapshot TV";
    tvChannel.provider = "Snapshot Provider";
    tvChannel.group = "Snapshot Group";
    tvChannel.radio = false;
    tvChannel.encrypted = false;
    tvChannel.enabled = true;
    snapshot.channels.push_back(tvChannel);

    VdrChannel radioChannel;
    radioChannel.id = "snapshot-channel-2";
    radioChannel.number = 2;
    radioChannel.name = "Snapshot Radio";
    radioChannel.provider = "Snapshot Provider";
    radioChannel.group = "Snapshot Group";
    radioChannel.radio = true;
    radioChannel.encrypted = true;
    radioChannel.enabled = true;
    snapshot.channels.push_back(radioChannel);

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

    VdrTimer activeTimer;
    activeTimer.id = "snapshot-timer-1";
    activeTimer.channelId = "snapshot-channel-1";
    activeTimer.eventId = "snapshot-event-1";
    activeTimer.title = "Snapshot Timer";
    activeTimer.subtitle = "Snapshot Timer Subtitle";
    activeTimer.startTime = "2026-06-04T20:00:00";
    activeTimer.endTime = "2026-06-04T21:00:00";
    activeTimer.priority = 50;
    activeTimer.lifetime = 99;
    activeTimer.enabled = true;
    activeTimer.recording = false;
    snapshot.timers.push_back(activeTimer);

    VdrTimer recordingTimer;
    recordingTimer.id = "snapshot-timer-2";
    recordingTimer.channelId = "snapshot-channel-1";
    recordingTimer.eventId = "snapshot-event-1";
    recordingTimer.title = "Snapshot Recording Timer";
    recordingTimer.subtitle = "Snapshot Recording Timer Subtitle";
    recordingTimer.startTime = "2026-06-04T20:00:00";
    recordingTimer.endTime = "2026-06-04T21:00:00";
    recordingTimer.priority = 50;
    recordingTimer.lifetime = 99;
    recordingTimer.enabled = false;
    recordingTimer.recording = true;
    snapshot.timers.push_back(recordingTimer);

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

static void test_vdr_overview_service_returns_status()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.status.enabled == true);
    assert(overview.status.mode == "mock");
    assert(overview.status.host == "mock");
    assert(overview.status.port == 0);
    assert(overview.status.state == "connected");
}

static void test_vdr_overview_service_counts_channels()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalChannels == 3);
    assert(overview.radioChannels == 1);
    assert(overview.encryptedChannels == 0);
}

static void test_vdr_overview_service_counts_events()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalEvents == 2);
}

static void test_vdr_overview_service_counts_timers()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalTimers == 1);
    assert(overview.activeTimers == 1);
    assert(overview.recordingTimers == 0);
}

static void test_vdr_overview_service_exposes_next_timer()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.hasNextTimer == true);
    assert(overview.nextTimer.id == "mock-timer-1");
    assert(overview.nextTimer.title == "Tagesschau");
    assert(overview.nextTimer.channelId == "mock-channel-1");
    assert(overview.nextTimer.eventId == "mock-event-1");
}

static void test_vdr_overview_service_counts_recordings()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalRecordings == 2);
}

static void test_vdr_overview_service_exposes_latest_recording()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.hasLatestRecording == true);
    assert(overview.latestRecording.id == "mock-recording-1");
    assert(overview.latestRecording.title == "Tagesschau");
}

static void test_vdr_overview_service_returns_empty_overview_without_snapshot()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);
    VdrOverviewService overviewService(accessService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalChannels == 0);
    assert(overview.radioChannels == 0);
    assert(overview.encryptedChannels == 0);
    assert(overview.totalEvents == 0);
    assert(overview.totalTimers == 0);
    assert(overview.activeTimers == 0);
    assert(overview.recordingTimers == 0);
    assert(overview.hasNextTimer == false);
    assert(overview.totalRecordings == 0);
    assert(overview.hasLatestRecording == false);
}

static void test_vdr_overview_service_uses_snapshot_access_service()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshot snapshot = makeSnapshotOverviewTestSnapshot();
    cache.update(snapshot);

    VdrOverviewService overviewService(accessService);
    VdrOverview overview = overviewService.getOverview();

    assert(overview.status.enabled == true);
    assert(overview.status.mode == "snapshot");
    assert(overview.status.host == "snapshot-host");
    assert(overview.status.port == 1234);
    assert(overview.status.state == "cached");

    assert(overview.totalChannels == 2);
    assert(overview.radioChannels == 1);
    assert(overview.encryptedChannels == 1);

    assert(overview.totalEvents == 1);

    assert(overview.totalTimers == 2);
    assert(overview.activeTimers == 1);
    assert(overview.recordingTimers == 1);
    assert(overview.hasNextTimer == true);
    assert(overview.nextTimer.id == "snapshot-timer-1");
    assert(overview.nextTimer.title == "Snapshot Timer");

    assert(overview.totalRecordings == 1);
    assert(overview.hasLatestRecording == true);
    assert(overview.latestRecording.id == "snapshot-recording-1");
    assert(overview.latestRecording.title == "Snapshot Recording");
}

int main()
{
    test_vdr_overview_service_returns_status();
    test_vdr_overview_service_counts_channels();
    test_vdr_overview_service_counts_events();
    test_vdr_overview_service_counts_timers();
    test_vdr_overview_service_exposes_next_timer();
    test_vdr_overview_service_counts_recordings();
    test_vdr_overview_service_exposes_latest_recording();
    test_vdr_overview_service_returns_empty_overview_without_snapshot();
    test_vdr_overview_service_uses_snapshot_access_service();

    return 0;
}
