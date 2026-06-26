#include "ApiRouter.h"
#include "SearchTimerPreviewEpgCache.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadService.h"

#include <cassert>
#include <string>
#include <vector>

static VdrEvent make_event(const std::string& id, const std::string& title)
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
    snapshot.events.push_back(make_event(eventId, eventTitle));
    return snapshot;
}

static void test_ready_cache_is_used_for_default_backend()
{
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    VdrSnapshotReadService snapshotReadService(snapshotAccessService);

    snapshotCache.update(make_snapshot("default", "snapshot-event", "Snapshot Event"));

    SearchTimerPreviewSnapshotReadFacade facade(snapshotReadService);
    SearchTimerPreviewEpgCache previewCache;
    previewCache.updateReady(
        "default",
        std::vector<VdrEvent>{make_event("cache-event", "Cache Event")});

    facade.setSearchTimerPreviewEpgCache(&previewCache);

    const std::vector<VdrEvent> events = facade.getEvents();

    assert(events.size() == 1);
    assert(events.at(0).id == "cache-event");
}

static void test_stale_cache_falls_back_to_snapshot()
{
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    VdrSnapshotReadService snapshotReadService(snapshotAccessService);

    snapshotCache.update(make_snapshot("default", "snapshot-event", "Snapshot Event"));

    SearchTimerPreviewSnapshotReadFacade facade(snapshotReadService);
    SearchTimerPreviewEpgCache previewCache;
    previewCache.updateReady(
        "default",
        std::vector<VdrEvent>{make_event("cache-event", "Cache Event")});
    previewCache.markStale("default");

    facade.setSearchTimerPreviewEpgCache(&previewCache);

    const std::vector<VdrEvent> events = facade.getEvents();

    assert(events.size() == 1);
    assert(events.at(0).id == "snapshot-event");
}

static void test_backend_cache_is_backend_scoped()
{
    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    VdrSnapshotReadService snapshotReadService(snapshotAccessService);

    snapshotCache.updateForBackend(
        "default",
        make_snapshot("default", "snapshot-default", "Default Snapshot"));
    snapshotCache.updateForBackend(
        "remote",
        make_snapshot("remote", "snapshot-remote", "Remote Snapshot"));

    SearchTimerPreviewSnapshotReadFacade facade(snapshotReadService);
    SearchTimerPreviewEpgCache previewCache;
    previewCache.updateReady(
        "remote",
        std::vector<VdrEvent>{make_event("cache-remote", "Remote Cache")});

    facade.setSearchTimerPreviewEpgCache(&previewCache);

    const std::vector<VdrEvent> defaultEvents = facade.getEventsForBackend("default");
    const std::vector<VdrEvent> remoteEvents = facade.getEventsForBackend("remote");

    assert(defaultEvents.size() == 1);
    assert(defaultEvents.at(0).id == "snapshot-default");

    assert(remoteEvents.size() == 1);
    assert(remoteEvents.at(0).id == "cache-remote");
}

int main()
{
    test_ready_cache_is_used_for_default_backend();
    test_stale_cache_falls_back_to_snapshot();
    test_backend_cache_is_backend_scoped();
    return 0;
}
