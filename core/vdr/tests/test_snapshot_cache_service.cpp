#include "SnapshotCacheService.h"

#include <cassert>
#include <vector>

static void test_service_exposes_existing_cache_boundary()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    assert(!service.cache().hasSnapshot());

    VdrSnapshot snapshot;
    service.cache().update(snapshot);

    assert(service.cache().hasSnapshot());

    service.cache().clear();

    assert(!service.cache().hasSnapshot());
}

static void test_update_snapshot_replaces_complete_snapshot()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    VdrSnapshot snapshot;
    snapshot.status.enabled = true;
    snapshot.status.mode = "test";
    snapshot.status.state = "connected";

    service.updateSnapshot(snapshot);

    assert(service.cache().hasSnapshot());
    assert(service.cache().snapshot().status.enabled == true);
    assert(service.cache().snapshot().status.mode == "test");
    assert(service.cache().snapshot().status.state == "connected");
}

static void test_update_status_updates_only_status_domain()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    VdrSnapshot snapshot;

    VdrRecording recording;
    recording.id = "recording-1";
    snapshot.recordings.push_back(recording);

    service.updateSnapshot(snapshot);

    VdrStatus status;
    status.enabled = true;
    status.mode = "partial";
    status.state = "connected";

    service.updateStatus(status);

    assert(service.cache().hasSnapshot());
    assert(service.cache().snapshot().status.enabled == true);
    assert(service.cache().snapshot().status.mode == "partial");
    assert(service.cache().snapshot().recordings.size() == 1);
    assert(service.cache().snapshot().recordings[0].id == "recording-1");
}

static void test_update_recordings_updates_only_recordings_domain()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    VdrSnapshot snapshot;
    snapshot.status.enabled = true;
    snapshot.status.mode = "test";

    service.updateSnapshot(snapshot);

    VdrRecording recording;
    recording.id = "recording-1";

    service.updateRecordings({ recording });

    assert(service.cache().hasSnapshot());
    assert(service.cache().snapshot().status.enabled == true);
    assert(service.cache().snapshot().status.mode == "test");
    assert(service.cache().snapshot().recordings.size() == 1);
    assert(service.cache().snapshot().recordings[0].id == "recording-1");
}

static void test_update_timers_updates_only_timers_domain()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    VdrTimer timer;
    timer.id = "timer-1";

    service.updateTimers({ timer });

    assert(service.cache().hasSnapshot());
    assert(service.cache().snapshot().timers.size() == 1);
    assert(service.cache().snapshot().timers[0].id == "timer-1");
}

static void test_update_channels_updates_only_channels_domain()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    VdrChannel channel;
    channel.id = "channel-1";

    service.updateChannels({ channel });

    assert(service.cache().hasSnapshot());
    assert(service.cache().snapshot().channels.size() == 1);
    assert(service.cache().snapshot().channels[0].id == "channel-1");
}

static void test_update_events_updates_only_events_domain()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    VdrEvent event;
    event.id = "event-1";

    service.updateEvents({ event });

    assert(service.cache().hasSnapshot());
    assert(service.cache().snapshot().events.size() == 1);
    assert(service.cache().snapshot().events[0].id == "event-1");
}

static void test_clear_clears_cache()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    VdrSnapshot snapshot;
    service.updateSnapshot(snapshot);

    assert(service.cache().hasSnapshot());

    service.clear();

    assert(!service.cache().hasSnapshot());
}

int main()
{
    test_service_exposes_existing_cache_boundary();
    test_update_snapshot_replaces_complete_snapshot();
    test_update_status_updates_only_status_domain();
    test_update_recordings_updates_only_recordings_domain();
    test_update_timers_updates_only_timers_domain();
    test_update_channels_updates_only_channels_domain();
    test_update_events_updates_only_events_domain();
    test_clear_clears_cache();

    return 0;
}
