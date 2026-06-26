#include "SearchTimer.h"
#include "SearchTimerPreviewEpgCache.h"
#include "SearchTimerPreviewEpgInputContext.h"
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

static SearchTimer makeSearchTimerWithQuery(
    const std::string& query)
{
    return SearchTimer::create(
        SearchTimerId::fromBackendNativeId(
            "home-vdr",
            "preview"),
        query + " Preview",
        query,
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
    assert(result.epgInputStatus() == "ready");
    assert(result.epgInputAvailable() == true);
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

static void test_preview_respects_offset()
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
            1);

    assert(result.totalCount() == 2);
    assert(result.returnedCount() == 1);
    assert(result.searchResult().matches().at(0).event().subtitle == "Episode B");
}

static void test_preview_respects_title_only_comparison_options()
{
    const std::vector<VdrEvent> events = {
        makeEvent(
            "1",
            "channel-a",
            "Amerika Spezial",
            "",
            "",
            "2026-06-21T20:15:00"),
        makeEvent(
            "2",
            "channel-b",
            "Reisebericht",
            "",
            "Amerika steht nur in der Beschreibung",
            "2026-06-21T21:15:00"),
        makeEvent(
            "3",
            "channel-c",
            "Weltpolitik",
            "Amerika im Untertitel",
            "",
            "2026-06-21T22:15:00")
    };

    SearchTimer searchTimer =
        makeSearchTimerWithQuery("Amerika");

    searchTimer.comparisonOptions().setCompareTitle(true);
    searchTimer.comparisonOptions().setCompareSubtitle(false);
    searchTimer.comparisonOptions().setCompareSummary(false);
    searchTimer.comparisonOptions().setCompareCategories(false);

    SearchTimerPreviewService service;

    const SearchTimerPreviewResult result =
        service.preview(
            searchTimer,
            events,
            0,
            0);

    assert(result.totalCount() == 1);
    assert(result.returnedCount() == 1);
    assert(result.searchResult().matches().at(0).event().title == "Amerika Spezial");
}

static void test_preview_respects_subtitle_only_comparison_options()
{
    const std::vector<VdrEvent> events = {
        makeEvent(
            "1",
            "channel-a",
            "Amerika Spezial",
            "",
            "",
            "2026-06-21T20:15:00"),
        makeEvent(
            "2",
            "channel-b",
            "Weltpolitik",
            "Amerika im Untertitel",
            "",
            "2026-06-21T21:15:00"),
        makeEvent(
            "3",
            "channel-c",
            "Reisebericht",
            "",
            "Amerika steht nur in der Beschreibung",
            "2026-06-21T22:15:00")
    };

    SearchTimer searchTimer =
        makeSearchTimerWithQuery("Amerika");

    searchTimer.comparisonOptions().setCompareTitle(false);
    searchTimer.comparisonOptions().setCompareSubtitle(true);
    searchTimer.comparisonOptions().setCompareSummary(false);
    searchTimer.comparisonOptions().setCompareCategories(false);

    SearchTimerPreviewService service;

    const SearchTimerPreviewResult result =
        service.preview(
            searchTimer,
            events,
            0,
            0);

    assert(result.totalCount() == 1);
    assert(result.returnedCount() == 1);
    assert(result.searchResult().matches().at(0).event().subtitle == "Amerika im Untertitel");
}

static void test_preview_respects_summary_only_comparison_options()
{
    const std::vector<VdrEvent> events = {
        makeEvent(
            "1",
            "channel-a",
            "Amerika Spezial",
            "",
            "",
            "2026-06-21T20:15:00"),
        makeEvent(
            "2",
            "channel-b",
            "Weltpolitik",
            "Amerika im Untertitel",
            "",
            "2026-06-21T21:15:00"),
        makeEvent(
            "3",
            "channel-c",
            "Reisebericht",
            "",
            "Amerika steht nur in der Beschreibung",
            "2026-06-21T22:15:00")
    };

    SearchTimer searchTimer =
        makeSearchTimerWithQuery("Amerika");

    searchTimer.comparisonOptions().setCompareTitle(false);
    searchTimer.comparisonOptions().setCompareSubtitle(false);
    searchTimer.comparisonOptions().setCompareSummary(true);
    searchTimer.comparisonOptions().setCompareCategories(false);

    SearchTimerPreviewService service;

    const SearchTimerPreviewResult result =
        service.preview(
            searchTimer,
            events,
            0,
            0);

    assert(result.totalCount() == 1);
    assert(result.returnedCount() == 1);
    assert(result.searchResult().matches().at(0).event().description == "Amerika steht nur in der Beschreibung");
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
    assert(json.find("\"epgInput\"") != std::string::npos);
    assert(json.find("\"status\":\"ready\"") != std::string::npos);
    assert(json.find("\"available\":true") != std::string::npos);
    assert(json.find("\"statistics\"") != std::string::npos);
    assert(json.find("\"preview\"") != std::string::npos);
    assert(json.find("\"totalCount\":1") != std::string::npos);
    assert(json.find("\"returnedCount\":1") != std::string::npos);
    assert(json.find("\"channelCount\":1") != std::string::npos);
}

static void test_preview_uses_non_ready_epg_input_context_once()
{
    SearchTimerPreviewEpgInputContext::setCacheStatus(
        SearchTimerPreviewEpgCacheStatus::Warming,
        "home-vdr");

    SearchTimerPreviewService service;

    const SearchTimerPreviewResult firstResult =
        service.preview(
            makeSearchTimer(),
            {},
            0,
            0);

    assert(firstResult.totalCount() == 0);
    assert(firstResult.epgInputStatus() == "warming");
    assert(firstResult.epgInputAvailable() == false);
    assert(firstResult.warnings().size() == 1);

    const SearchTimerPreviewResult secondResult =
        service.preview(
            makeSearchTimer(),
            {},
            0,
            0);

    assert(secondResult.epgInputStatus() == "ready");
    assert(secondResult.epgInputAvailable() == true);
    assert(secondResult.warnings().empty());
}

int main()
{
    test_preview_matches_events_by_search_timer_query();
    test_preview_respects_limit();
    test_preview_respects_offset();
    test_preview_respects_title_only_comparison_options();
    test_preview_respects_subtitle_only_comparison_options();
    test_preview_respects_summary_only_comparison_options();
    test_preview_json_contains_statistics_and_preview();
    test_preview_uses_non_ready_epg_input_context_once();

    std::cout << "test_search_timer_preview_service passed" << std::endl;
    return 0;
}
