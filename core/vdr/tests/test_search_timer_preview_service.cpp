#include "SearchTimerPreviewService.h"

#include <cassert>
#include <iostream>

static VdrEvent makeEvent(
    const std::string& id,
    const std::string& title,
    const std::string& subtitle,
    const std::string& description,
    const std::string& startTime)
{
    VdrEvent event;
    event.id = id;
    event.channelId = "channel-1";
    event.title = title;
    event.subtitle = subtitle;
    event.description = description;
    event.startTime = startTime;
    event.endTime = startTime;
    event.durationSeconds = 3600;
    return event;
}

static SearchTimer makeSearchTimer()
{
    return SearchTimer::create(
        SearchTimerId::fromBackendNativeId("home-vdr", "searchtimer-1"),
        "Terra X Suche",
        "Terra X",
        SearchTimerState::Active);
}

static void test_preview_matches_future_events_by_search_timer_query()
{
    SearchTimerPreviewService service;
    SearchTimer searchTimer = makeSearchTimer();

    const auto result =
        service.preview(
            searchTimer,
            {
                makeEvent("event-1", "Terra X", "", "", "1000"),
                makeEvent("event-2", "Tagesschau", "", "", "1100"),
                makeEvent("event-3", "", "Terra Xpress", "", "1200")
            });

    assert(result.searchTimer().backendNativeId() == "searchtimer-1");
    assert(result.searchTimer().name() == "Terra X Suche");
    assert(result.totalCount() == 2);
    assert(result.returnedCount() == 2);
    assert(result.empty() == false);
    assert(result.searchResult().matches()[0].event().id == "event-1");
    assert(result.searchResult().matches()[1].event().id == "event-3");
}

static void test_preview_supports_limit_and_offset()
{
    SearchTimerPreviewService service;
    SearchTimer searchTimer = makeSearchTimer();

    const auto result =
        service.preview(
            searchTimer,
            {
                makeEvent("event-1", "Terra X 1", "", "", "1000"),
                makeEvent("event-2", "Terra X 2", "", "", "1100"),
                makeEvent("event-3", "Terra X 3", "", "", "1200")
            },
            1,
            1);

    assert(result.totalCount() == 3);
    assert(result.returnedCount() == 1);
    assert(result.searchResult().limit() == 1);
    assert(result.searchResult().offset() == 1);
    assert(result.searchResult().matches()[0].event().id == "event-2");
}

static void test_preview_returns_empty_result_without_matches()
{
    SearchTimerPreviewService service;
    SearchTimer searchTimer = makeSearchTimer();

    const auto result =
        service.preview(
            searchTimer,
            {
                makeEvent("event-1", "Tagesschau", "", "", "1000")
            });

    assert(result.totalCount() == 0);
    assert(result.returnedCount() == 0);
    assert(result.empty() == true);
}

int main()
{
    test_preview_matches_future_events_by_search_timer_query();
    test_preview_supports_limit_and_offset();
    test_preview_returns_empty_result_without_matches();

    std::cout
        << "test_search_timer_preview_service passed"
        << std::endl;

    return 0;
}
