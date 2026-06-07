#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadService.h"

#include <cassert>

static VdrSnapshot makeTestSnapshot()
{
    VdrSnapshot snapshot;

    snapshot.status.enabled = true;
    snapshot.status.mode = "snapshot-read-test";
    snapshot.status.host = "snapshot-read-host";
    snapshot.status.port = 1234;
    snapshot.status.state = "connected";

    VdrChannel channel;
    channel.id = "channel-1";
    channel.name = "Channel One";
    snapshot.channels.push_back(channel);

    VdrEvent event;
    event.id = "event-1";
    event.title = "Event One";
    snapshot.events.push_back(event);

    VdrTimer timer;
    timer.id = "timer-1";
    timer.enabled = true;
    snapshot.timers.push_back(timer);

    VdrRecording recording;
    recording.id = "recording-1";
    recording.title = "Recording One";
    snapshot.recordings.push_back(recording);

    return snapshot;
}

static void test_snapshot_read_service_without_snapshot()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    VdrSnapshotReadService readService(accessService);

    assert(!readService.hasSnapshot());

    assert(readService.getChannels().empty());
    assert(readService.getEvents().empty());
    assert(readService.getTimers().empty());
    assert(readService.getRecordings().empty());
}

static void test_snapshot_read_service_with_snapshot()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    cache.update(makeTestSnapshot());

    VdrSnapshotReadService readService(accessService);

    assert(readService.hasSnapshot());

    assert(readService.getChannels().size() == 1);
    assert(readService.getEvents().size() == 1);
    assert(readService.getTimers().size() == 1);
    assert(readService.getRecordings().size() == 1);

    assert(readService.getChannels()[0].id == "channel-1");
    assert(readService.getEvents()[0].id == "event-1");
    assert(readService.getTimers()[0].id == "timer-1");
    assert(readService.getRecordings()[0].id == "recording-1");
}

int main()
{
    test_snapshot_read_service_without_snapshot();
    test_snapshot_read_service_with_snapshot();

    return 0;
}
