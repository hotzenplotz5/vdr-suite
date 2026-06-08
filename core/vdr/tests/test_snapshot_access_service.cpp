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

    assert(cachedSnapshot->recordings.size() == 1);
    assert(cachedSnapshot->recordings.front().id == "snapshot-recording-1");
    assert(cachedSnapshot->recordings.front().title == "Snapshot Recording");
}

int main()
{
    test_snapshot_access_service_returns_empty_state();
    test_snapshot_access_service_returns_populated_snapshot();

    return 0;
}
