#include "SearchTimer.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrSnapshot.h"

#include <cassert>

static VdrSnapshot makeTestSnapshot()
{
    VdrSnapshot snapshot;

    snapshot.status.enabled = true;
    snapshot.status.mode = "snapshot-test";
    snapshot.status.host = "snapshot-host";
    snapshot.status.port = 1234;
    snapshot.status.state = "cached";

    VdrChannel channel;
    channel.id = "snapshot-channel-1";
    channel.number = 1;
    channel.name = "Snapshot Channel";
    channel.provider = "Snapshot Provider";
    channel.group = "Snapshot Group";
    channel.radio = true;
    channel.encrypted = false;
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
    timer.recording = false;
    snapshot.timers.push_back(timer);

    SearchTimer searchTimer = SearchTimer::create(
        SearchTimerId::fromBackendNativeId("default", "searchtimer-1"),
        "Snapshot SearchTimer",
        "Terra X",
        SearchTimerState::Active);
    snapshot.searchTimers.push_back(searchTimer);

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

static void test_snapshot_access_service_returns_empty_state()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    assert(!accessService.hasSnapshot());
    assert(accessService.snapshot() == nullptr);
}

static void test_snapshot_access_service_returns_populated_snapshot()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshot snapshot = makeTestSnapshot();
    cache.update(snapshot);

    assert(accessService.hasSnapshot());
    assert(accessService.snapshot() != nullptr);

    const VdrSnapshot* cachedSnapshot = accessService.snapshot();

    assert(cachedSnapshot->status.enabled == true);
    assert(cachedSnapshot->status.mode == "snapshot-test");
    assert(cachedSnapshot->status.host == "snapshot-host");
    assert(cachedSnapshot->status.port == 1234);
    assert(cachedSnapshot->status.state == "cached");

    assert(cachedSnapshot->channels.size() == 1);
    assert(cachedSnapshot->channels.front().id == "snapshot-channel-1");
    assert(cachedSnapshot->channels.front().radio == true);
    assert(cachedSnapshot->channels.front().encrypted == false);

    assert(cachedSnapshot->events.size() == 1);
    assert(cachedSnapshot->events.front().id == "snapshot-event-1");
    assert(cachedSnapshot->events.front().title == "Snapshot Event");

    assert(cachedSnapshot->timers.size() == 1);
    assert(cachedSnapshot->timers.front().id == "snapshot-timer-1");
    assert(cachedSnapshot->timers.front().enabled == true);
    assert(cachedSnapshot->timers.front().recording == false);

    assert(cachedSnapshot->searchTimers.size() == 1);
    assert(cachedSnapshot->searchTimers.front().backendNativeId() == "searchtimer-1");
    assert(cachedSnapshot->searchTimers.front().name() == "Snapshot SearchTimer");
    assert(cachedSnapshot->searchTimers.front().query() == "Terra X");
    assert(cachedSnapshot->searchTimers.front().isActive());

    assert(cachedSnapshot->recordings.size() == 1);
    assert(cachedSnapshot->recordings.front().id == "snapshot-recording-1");
    assert(cachedSnapshot->recordings.front().title == "Snapshot Recording");
}

static void test_snapshot_access_service_returns_empty_backend_state()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    assert(!accessService.hasSnapshotForBackend("default"));
    assert(accessService.snapshotForBackend("default") == nullptr);
}

static void test_snapshot_access_service_returns_matching_backend_snapshot()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshot snapshot = makeTestSnapshot();
    snapshot.backendId = "parents-vdr";
    cache.update(snapshot);

    assert(accessService.hasSnapshotForBackend("parents-vdr"));
    assert(accessService.snapshotForBackend("parents-vdr") != nullptr);

    const VdrSnapshot* cachedSnapshot = accessService.snapshotForBackend("parents-vdr");

    assert(cachedSnapshot->backendId == "parents-vdr");
    assert(cachedSnapshot->status.mode == "snapshot-test");
    assert(cachedSnapshot->channels.size() == 1);
    assert(cachedSnapshot->searchTimers.size() == 1);
    assert(cachedSnapshot->searchTimers.front().name() == "Snapshot SearchTimer");
    assert(cachedSnapshot->recordings.size() == 1);
}

static void test_snapshot_access_service_rejects_unknown_backend_snapshot()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshot snapshot = makeTestSnapshot();
    snapshot.backendId = "parents-vdr";
    cache.update(snapshot);

    assert(!accessService.hasSnapshotForBackend("home-vdr"));
    assert(accessService.snapshotForBackend("home-vdr") == nullptr);
}


static void test_snapshot_access_service_reads_distinct_backend_snapshots()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshot parentsSnapshot = makeTestSnapshot();
    parentsSnapshot.backendId = "parents-vdr";
    parentsSnapshot.status.host = "parents-host";

    VdrSnapshot homeSnapshot = makeTestSnapshot();
    homeSnapshot.backendId = "home-vdr";
    homeSnapshot.status.host = "home-host";

    cache.updateForBackend("parents-vdr", parentsSnapshot);
    cache.updateForBackend("home-vdr", homeSnapshot);

    assert(accessService.hasSnapshotForBackend("parents-vdr"));
    assert(accessService.hasSnapshotForBackend("home-vdr"));

    const VdrSnapshot* parents =
        accessService.snapshotForBackend("parents-vdr");

    const VdrSnapshot* home =
        accessService.snapshotForBackend("home-vdr");

    assert(parents != nullptr);
    assert(home != nullptr);

    assert(parents->backendId == "parents-vdr");
    assert(parents->status.host == "parents-host");

    assert(home->backendId == "home-vdr");
    assert(home->status.host == "home-host");
}

int main()
{
    test_snapshot_access_service_returns_empty_state();
    test_snapshot_access_service_returns_populated_snapshot();
    test_snapshot_access_service_returns_empty_backend_state();
    test_snapshot_access_service_returns_matching_backend_snapshot();
    test_snapshot_access_service_rejects_unknown_backend_snapshot();
    test_snapshot_access_service_reads_distinct_backend_snapshots();

    return 0;
}
