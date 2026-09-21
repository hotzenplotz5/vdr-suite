#include "SearchTimerPreviewEpgCacheChangeInvalidator.h"

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

static void test_event_domain_change_marks_ready_cache_stale()
{
    SearchTimerPreviewEpgCache cache;

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Amerika")});

    const bool invalidated = SearchTimerPreviewEpgCacheChangeInvalidator::invalidateForChangeEvents(
        cache,
        "default",
        std::vector<VdrChangeEvent>{
            VdrChangeEvent(VdrChangeType::EventsChanged)});

    assert(invalidated);
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Stale);
    assert(cache.eventsForBackend("default") == nullptr);
}

static void test_timer_change_does_not_invalidate_ready_cache()
{
    SearchTimerPreviewEpgCache cache;

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Amerika")});

    const bool invalidated = SearchTimerPreviewEpgCacheChangeInvalidator::invalidateForChangeEvents(
        cache,
        "default",
        std::vector<VdrChangeEvent>{
            VdrChangeEvent(VdrChangeType::TimersChanged)});

    assert(!invalidated);
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Ready);
}

static void test_unknown_cache_is_not_created_as_stale()
{
    SearchTimerPreviewEpgCache cache;

    const bool invalidated = SearchTimerPreviewEpgCacheChangeInvalidator::invalidateForChangeEvents(
        cache,
        "default",
        std::vector<VdrChangeEvent>{
            VdrChangeEvent(VdrChangeType::EventsChanged)});

    assert(!invalidated);
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Unknown);
    assert(cache.backendIds().empty());
}

static void test_invalidation_is_backend_scoped()
{
    SearchTimerPreviewEpgCache cache;

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Default")});
    cache.updateReady(
        "remote",
        std::vector<VdrEvent>{
            make_event("2", "Remote")});

    const bool invalidated = SearchTimerPreviewEpgCacheChangeInvalidator::invalidateForChangeEvents(
        cache,
        "remote",
        std::vector<VdrChangeEvent>{
            VdrChangeEvent(VdrChangeType::EventsChanged)});

    assert(invalidated);
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Ready);
    assert(cache.statusForBackend("remote") ==
        SearchTimerPreviewEpgCacheStatus::Stale);
}

int main()
{
    test_event_domain_change_marks_ready_cache_stale();
    test_timer_change_does_not_invalidate_ready_cache();
    test_unknown_cache_is_not_created_as_stale();
    test_invalidation_is_backend_scoped();

    return 0;
}
