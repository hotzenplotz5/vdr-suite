#include "SearchTimerPreviewEpgCache.h"

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

static void test_missing_backend_is_unknown_not_empty_ready_cache()
{
    SearchTimerPreviewEpgCache cache;

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Unknown);
    assert(!cache.isReadyForBackend("default"));
    assert(cache.eventsForBackend("default") == nullptr);
    assert(cache.readyEventCountForBackend("default") == 0);
}

static void test_ready_empty_cache_is_distinct_from_unknown()
{
    SearchTimerPreviewEpgCache cache;

    cache.updateReady("default", std::vector<VdrEvent>{});

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Ready);
    assert(cache.isReadyForBackend("default"));
    assert(cache.eventsForBackend("default") != nullptr);
    assert(cache.eventsForBackend("default")->empty());
    assert(cache.readyEventCountForBackend("default") == 0);
}

static void test_backend_scoped_ready_events()
{
    SearchTimerPreviewEpgCache cache;

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Amerika")});

    cache.updateReady(
        "ferienhaus",
        std::vector<VdrEvent>{
            make_event("2", "Tagesschau"),
            make_event("3", "Tatort")});

    const std::vector<VdrEvent>* defaultEvents =
        cache.eventsForBackend("");
    const std::vector<VdrEvent>* ferienhausEvents =
        cache.eventsForBackend("ferienhaus");

    assert(defaultEvents != nullptr);
    assert(ferienhausEvents != nullptr);

    assert(defaultEvents->size() == 1);
    assert(defaultEvents->at(0).title == "Amerika");

    assert(ferienhausEvents->size() == 2);
    assert(ferienhausEvents->at(1).title == "Tatort");

    assert(cache.backendIds().size() == 2);
}

static void test_non_ready_statuses_do_not_expose_events_as_ready()
{
    SearchTimerPreviewEpgCache cache;

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Amerika")});

    cache.markStale("default");

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Stale);
    assert(!cache.isReadyForBackend("default"));
    assert(cache.eventsForBackend("default") == nullptr);

    cache.markWarming("default");

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Warming);
    assert(!cache.isReadyForBackend("default"));
    assert(cache.eventsForBackend("default") == nullptr);

    cache.markUnavailable("default");

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Unavailable);
    assert(!cache.isReadyForBackend("default"));
    assert(cache.eventsForBackend("default") == nullptr);
}

static void test_clear_backend_returns_to_unknown()
{
    SearchTimerPreviewEpgCache cache;

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Amerika")});

    cache.clearBackend("default");

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Unknown);
    assert(cache.eventsForBackend("default") == nullptr);

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("2", "Tagesschau")});

    cache.clear();

    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Unknown);
    assert(cache.backendIds().empty());
}

int main()
{
    test_missing_backend_is_unknown_not_empty_ready_cache();
    test_ready_empty_cache_is_distinct_from_unknown();
    test_backend_scoped_ready_events();
    test_non_ready_statuses_do_not_expose_events_as_ready();
    test_clear_backend_returns_to_unknown();

    return 0;
}
