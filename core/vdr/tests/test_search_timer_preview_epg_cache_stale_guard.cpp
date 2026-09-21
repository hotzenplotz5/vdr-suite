#include "SearchTimerPreviewEpgCacheStaleGuard.h"

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

static void test_ready_cache_moves_to_stale()
{
    SearchTimerPreviewEpgCache cache;

    cache.updateReady(
        "default",
        std::vector<VdrEvent>{
            make_event("1", "Amerika")});

    const bool changed = SearchTimerPreviewEpgCacheStaleGuard::markBackendStaleWhenReady(
        cache,
        "default");

    assert(changed);
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Stale);
    assert(cache.eventsForBackend("default") == nullptr);
}

static void test_unknown_cache_is_not_created_as_stale()
{
    SearchTimerPreviewEpgCache cache;

    const bool changed = SearchTimerPreviewEpgCacheStaleGuard::markBackendStaleWhenReady(
        cache,
        "default");

    assert(!changed);
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Unknown);
    assert(cache.backendIds().empty());
}

static void test_non_ready_states_are_not_changed()
{
    SearchTimerPreviewEpgCache cache;

    cache.markWarming("default");
    assert(!SearchTimerPreviewEpgCacheStaleGuard::markBackendStaleWhenReady(
        cache,
        "default"));
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Warming);

    cache.markUnavailable("default");
    assert(!SearchTimerPreviewEpgCacheStaleGuard::markBackendStaleWhenReady(
        cache,
        "default"));
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Unavailable);

    cache.markStale("default");
    assert(!SearchTimerPreviewEpgCacheStaleGuard::markBackendStaleWhenReady(
        cache,
        "default"));
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Stale);
}

static void test_transition_is_backend_scoped()
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

    const bool changed = SearchTimerPreviewEpgCacheStaleGuard::markBackendStaleWhenReady(
        cache,
        "remote");

    assert(changed);
    assert(cache.statusForBackend("default") ==
        SearchTimerPreviewEpgCacheStatus::Ready);
    assert(cache.statusForBackend("remote") ==
        SearchTimerPreviewEpgCacheStatus::Stale);
}

int main()
{
    test_ready_cache_moves_to_stale();
    test_unknown_cache_is_not_created_as_stale();
    test_non_ready_states_are_not_changed();
    test_transition_is_backend_scoped();

    return 0;
}
