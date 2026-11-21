#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadService.h"

#include <cassert>

static VdrSnapshot makeTestSnapshot()
{
    VdrSnapshot snapshot;
    snapshot.backendId = "default";

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


static VdrSnapshot makeBackendSnapshot(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& recordingId)
{
    VdrSnapshot snapshot;
    snapshot.backendId = backendId;

    snapshot.status.enabled = true;
    snapshot.status.mode = "multi-backend-read-test";
    snapshot.status.host = backendId + "-host";
    snapshot.status.port = 8002;
    snapshot.status.state = "connected";

    VdrChannel channel;
    channel.id = channelId;
    channel.name = backendId + " Channel";
    snapshot.channels.push_back(channel);

    VdrEvent event;
    event.id = backendId + "-event";
    event.title = backendId + " Event";
    snapshot.events.push_back(event);

    VdrRecording recording;
    recording.id = recordingId;
    recording.title = backendId + " Recording";
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


static void test_snapshot_read_service_reads_matching_backend()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    cache.update(makeTestSnapshot());

    VdrSnapshotReadService readService(accessService);

    assert(readService.hasSnapshotForBackend("default"));
    assert(readService.getChannelsForBackend("default").size() == 1);
    assert(readService.getEventsForBackend("default").size() == 1);
    assert(readService.getTimersForBackend("default").size() == 1);
    assert(readService.getRecordingsForBackend("default").size() == 1);
    assert(readService.getStatusForBackend("default").state == "connected");
}

static void test_snapshot_read_service_rejects_non_matching_backend()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    cache.update(makeTestSnapshot());

    VdrSnapshotReadService readService(accessService);

    assert(!readService.hasSnapshotForBackend("ferienhaus"));
    assert(readService.getChannelsForBackend("ferienhaus").empty());
    assert(readService.getEventsForBackend("ferienhaus").empty());
    assert(readService.getTimersForBackend("ferienhaus").empty());
    assert(readService.getRecordingsForBackend("ferienhaus").empty());
    assert(readService.getStatusForBackend("ferienhaus").enabled == false);
}


static void test_snapshot_read_service_returns_all_backend_snapshots()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    cache.updateForBackend(
        "home-vdr",
        makeBackendSnapshot("home-vdr", "home-channel", "home-recording"));
    cache.updateForBackend(
        "parents-vdr",
        makeBackendSnapshot("parents-vdr", "parents-channel", "parents-recording"));

    VdrSnapshotReadService readService(accessService);

    const auto snapshots = readService.getSnapshots();

    assert(snapshots.size() == 2);

    bool foundHome = false;
    bool foundParents = false;

    for (const auto& snapshot : snapshots)
    {
        if (snapshot.backendId == "home-vdr")
        {
            foundHome = true;
            assert(snapshot.channels.size() == 1);
            assert(snapshot.channels[0].id == "home-channel");
            assert(snapshot.events.size() == 1);
            assert(snapshot.events[0].id == "home-vdr-event");
            assert(snapshot.recordings.size() == 1);
            assert(snapshot.recordings[0].id == "home-recording");
        }

        if (snapshot.backendId == "parents-vdr")
        {
            foundParents = true;
            assert(snapshot.channels.size() == 1);
            assert(snapshot.channels[0].id == "parents-channel");
            assert(snapshot.events.size() == 1);
            assert(snapshot.events[0].id == "parents-vdr-event");
            assert(snapshot.recordings.size() == 1);
            assert(snapshot.recordings[0].id == "parents-recording");
        }
    }

    assert(foundHome);
    assert(foundParents);
}

static void test_snapshot_read_service_ignores_preview_cache_for_event_reads()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    cache.update(makeTestSnapshot());

    VdrEvent cachedEvent;
    cachedEvent.id = "preview-cache-event";
    cachedEvent.title = "Preview Cache Event";

    accessService.searchTimerPreviewEpgCache().updateReady(
        "default",
        std::vector<VdrEvent>{cachedEvent});

    VdrSnapshotReadService readService(accessService);

    const auto events = readService.getEvents();

    assert(events.size() == 1);
    assert(events[0].id == "event-1");
    assert(readService.searchTimerPreviewEpgCache().readyEventCountForBackend("default") == 1);
}

static void test_snapshot_read_service_ignores_preview_cache_for_backend_event_reads()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    cache.updateForBackend(
        "home-vdr",
        makeBackendSnapshot("home-vdr", "home-channel", "home-recording"));

    VdrEvent cachedEvent;
    cachedEvent.id = "home-preview-cache-event";
    cachedEvent.title = "Home Preview Cache Event";

    accessService.searchTimerPreviewEpgCache().updateReady(
        "home-vdr",
        std::vector<VdrEvent>{cachedEvent});

    VdrSnapshotReadService readService(accessService);

    const auto events = readService.getEventsForBackend("home-vdr");

    assert(events.size() == 1);
    assert(events[0].id == "home-vdr-event");
    assert(readService.searchTimerPreviewEpgCache().readyEventCountForBackend("home-vdr") == 1);
}


int main()
{
    test_snapshot_read_service_without_snapshot();
    test_snapshot_read_service_with_snapshot();
    test_snapshot_read_service_reads_matching_backend();
    test_snapshot_read_service_rejects_non_matching_backend();
    test_snapshot_read_service_returns_all_backend_snapshots();
    test_snapshot_read_service_ignores_preview_cache_for_event_reads();
    test_snapshot_read_service_ignores_preview_cache_for_backend_event_reads();

    return 0;
}
