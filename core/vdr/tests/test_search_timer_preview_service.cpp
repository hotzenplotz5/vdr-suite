#include "SearchTimer.h"
#include "SearchTimerPreviewResultJsonSerializer.h"
#include "SearchTimerPreviewService.h"
#include "VdrEvent.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

static VdrEvent makeEvent(
    const std::string& id,
    const std::string& channelId,
    const std::string& title,
    const std::string& subtitle,
    const std::string& description,
    const std::string& startTime)
{
    VdrEvent event;
    event.id = id;
    event.channelId = channelId;
    event.title = title;
    event.subtitle = subtitle;
    event.description = description;
    event.startTime = startTime;
    event.endTime = "";
    event.durationSeconds = 3600;
    event.parentalRating = 0;
    return event;
}

static SearchTimer makeSearchTimer()
{
    return SearchTimer::create(
        SearchTimerId::fromBackendNativeId(
            "home-vdr",
            "preview"),
        "Tatort Preview",
        "Tatort",
        SearchTimerState::Active);
}

static void test_preview_matches_events_by_search_timer_query()
{
    const std::vector<VdrEvent> events = {
        makeEvent(
            "1",
            "channel-a",
            "Tatort",
            "Episode A",
            "Krimi",
            "2026-06-21T20:15:00"),
        makeEvent(
            "2",
            "channel-b",
            "Tagesschau",
            "",
            "Nachrichten",
            "2026-06-21T20:00:00")
    };

    SearchTimerPreviewService service;

    const SearchTimerPreviewResult result =
        service.preview(
            makeSearchTimer(),
            events,
            0,
            0);

    assert(result.empty() == false);
    assert(result.totalCount() == 1);
    assert(result.returnedCount() == 1);
    assert(result.searchTimer().query() == "Tatort");
    assert(result.searchResult().matches().at(0).event().title == "Tatort");
}

static void test_preview_respects_limit()
{
    const std::vector<VdrEvent> events = {
        makeEvent(
            "1",
            "channel-a",
            "Tatort",
            "Episode A",
            "Krimi",
            "2026-06-21T20:15:00"),
        makeEvent(
            "2",
            "channel-b",
            "Tatort",
            "Episode B",
            "Krimi",
            "2026-06-22T20:15:00")
    };

    SearchTimerPreviewService service;

    const SearchTimerPreviewResult result =
        service.preview(
            makeSearchTimer(),
            events,
            1,
            0);

    assert(result.totalCount() == 2);
    assert(result.returnedCount() == 1);
}

static void test_preview_json_contains_statistics_and_preview()
{
    const std::vector<VdrEvent> events = {
        makeEvent(
            "1",
            "channel-a",
            "Tatort",
            "Episode A",
            "Krimi",
            "2026-06-21T20:15:00")
    };

    SearchTimerPreviewService service;
    SearchTimerPreviewResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(
            service.preview(
                makeSearchTimer(),
                events,
                0,
                0));

    assert(json.find("\"searchTimer\"") != std::string::npos);
    assert(json.find("\"statistics\"") != std::string::npos);
    assert(json.find("\"preview\"") != std::string::npos);
    assert(json.find("\"totalCount\":1") != std::string::npos);
    assert(json.find("\"returnedCount\":1") != std::string::npos);
    assert(json.find("\"channelCount\":1") != std::string::npos);
}

int main()
{
    test_preview_matches_events_by_search_timer_query();
    test_preview_respects_limit();
    test_preview_json_contains_statistics_and_preview();

    std::cout << "test_search_timer_preview_service passed" << std::endl;
    return 0;
}
