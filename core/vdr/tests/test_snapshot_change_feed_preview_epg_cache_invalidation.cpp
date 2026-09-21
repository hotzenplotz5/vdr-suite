#include "SearchTimerPreviewEpgCacheChangeInvalidator.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedService.h"

#include <cassert>
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

static void test_append_events_change_invalidates_registered_ready_cache()
{
    SearchTimerPreviewEpgCache cache;
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    SearchTimerPreviewEpgCacheChangeInvalidator::registerRuntimeCache(cache);

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Amerika")});

    service.appendChanges(
        feed,
        1,
        std::vector<VdrChangeEvent>{
            VdrChangeEvent(VdrChangeType::EventsChanged)},
        "default");

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Stale);
    assert(cache.eventsForBackend("default") == nullptr);
    assert(feed.latestSequenceNumber() == 1);

    SearchTimerPreviewEpgCacheChangeInvalidator::clearRuntimeCache();
}

static void test_append_timer_change_does_not_invalidate_registered_ready_cache()
{
    SearchTimerPreviewEpgCache cache;
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    SearchTimerPreviewEpgCacheChangeInvalidator::registerRuntimeCache(cache);

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Amerika")});

    service.appendChanges(
        feed,
        1,
        std::vector<VdrChangeEvent>{
            VdrChangeEvent(VdrChangeType::TimersChanged)},
        "default");

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Ready);
    assert(feed.latestSequenceNumber() == 1);

    SearchTimerPreviewEpgCacheChangeInvalidator::clearRuntimeCache();
}

static void test_append_events_change_without_registered_cache_is_safe()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    SearchTimerPreviewEpgCacheChangeInvalidator::clearRuntimeCache();

    service.appendChanges(
        feed,
        1,
        std::vector<VdrChangeEvent>{
            VdrChangeEvent(VdrChangeType::EventsChanged)},
        "default");

    assert(feed.latestSequenceNumber() == 1);
}

int main()
{
    test_append_events_change_invalidates_registered_ready_cache();
    test_append_timer_change_does_not_invalidate_registered_ready_cache();
    test_append_events_change_without_registered_cache_is_safe();

    SearchTimerPreviewEpgCacheChangeInvalidator::clearRuntimeCache();

    return 0;
}
