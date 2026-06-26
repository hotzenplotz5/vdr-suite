#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadService.h"

#include <cassert>
#include <string>
#include <vector>

static VdrEvent make_event(
    const std::string& id,
    const std::string& title)
{
    VdrEvent event;

    event.id = id;
    event.title = title;
    event.durationSeconds = 0;
    event.parentalRating = 0;

    return event;
}

static VdrSnapshot make_snapshot(
    const std::string& backendId,
    const std::string& eventId,
    const std::string& eventTitle)
{
    VdrSnapshot snapshot;

    snapshot.backendId = backendId;
    snapshot.events.push_back(
        make_event(eventId, eventTitle));

    return snapshot;
}

static void test_unknown_cache_falls_back_to_snapshot_events()
{
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    VdrSnapshotReadService readService(snapshotAccessService);

    snapshotCache.update(
        make_snapshot(
            "default",
            "snapshot-event",
            "Snapshot Event"));

    const std::vector<VdrEvent> events =
        readService.getEvents();

    assert(events.size() == 1);
    assert(events.at(0).id == "snapshot-event");
}

static void test_ready_empty_cache_is_valid_zero_results()
{
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    VdrSnapshotReadService readService(snapshotAccessService);

    snapshotCache.update(
        make_snapshot(
            "default",
            "snapshot-event",
            "Snapshot Event"));

    readService.searchTimerPreviewEpgCache().updateReady(
        "default",
        std::vector<VdrEvent>{});

    const std::vector<VdrEvent> events =
        readService.getEvents();

    assert(events.empty());
}

static void test_ready_cache_overrides_snapshot_events()
{
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    VdrSnapshotReadService readService(snapshotAccessService);

    snapshotCache.update(
        make_snapshot(
            "default",
            "snapshot-event",
            "Snapshot Event"));

    readService.searchTimerPreviewEpgCache().updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("cache-event", "Cache Event")});

    const std::vector<VdrEvent> events =
        readService.getEvents();

    assert(events.size() == 1);
    assert(events.at(0).id == "cache-event");
}

static void test_stale_cache_falls_back_to_snapshot_events()
{
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    VdrSnapshotReadService readService(snapshotAccessService);

    snapshotCache.update(
        make_snapshot(
            "default",
            "snapshot-event",
            "Snapshot Event"));

    readService.searchTimerPreviewEpgCache().updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("cache-event", "Cache Event")});
    readService.searchTimerPreviewEpgCache().markStale("default");

    const std::vector<VdrEvent> events =
        readService.getEvents();

    assert(events.size() == 1);
    assert(events.at(0).id == "snapshot-event");
}

static void test_backend_cache_is_backend_scoped()
{
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    VdrSnapshotReadService readService(snapshotAccessService);

    snapshotCache.updateForBackend(
        "default",
        make_snapshot(
            "default",
            "snapshot-default",
            "Default Snapshot"));
    snapshotCache.updateForBackend(
        "remote",
        make_snapshot(
            "remote",
            "snapshot-remote",
            "Remote Snapshot"));

    readService.searchTimerPreviewEpgCache().updateReady(
        "remote",
        std::vector<VdrEvent>{
            make_event("cache-remote", "Remote Cache")});

    const std::vector<VdrEvent> defaultEvents =
        readService.getEventsForBackend("default");
    const std::vector<VdrEvent> remoteEvents =
        readService.getEventsForBackend("remote");

    assert(defaultEvents.size() == 1);
    assert(defaultEvents.at(0).id == "snapshot-default");

    assert(remoteEvents.size() == 1);
    assert(remoteEvents.at(0).id == "cache-remote");
}

int main()
{
    test_unknown_cache_falls_back_to_snapshot_events();
    test_ready_empty_cache_is_valid_zero_results();
    test_ready_cache_overrides_snapshot_events();
    test_stale_cache_falls_back_to_snapshot_events();
    test_backend_cache_is_backend_scoped();

    return 0;
}
