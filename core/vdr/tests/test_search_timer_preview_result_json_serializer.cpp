#include "SearchTimerPreviewResultJsonSerializer.h"

#include "EpgSearchMatch.h"
#include "EpgSearchResult.h"
#include "SearchTimer.h"

#include <cassert>
#include <iostream>

static VdrEvent makeEvent()
{
    VdrEvent event;
    event.id = "event-1";
    event.channelId = "channel-1";
    event.title = "Terra X";
    event.subtitle = "Rätsel alter Welten";
    event.description = "Eine Dokumentation";
    event.startTime = "1000";
    event.endTime = "1100";
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

static EpgSearchResult makeSearchResult()
{
    return EpgSearchResult(
        {
            EpgSearchMatch(
                makeEvent(),
                "home-vdr",
                {"title"})
        },
        1,
        10,
        0);
}

static void test_default_result_reports_ready_epg_input()
{
    SearchTimerPreviewResult previewResult(
        makeSearchTimer(),
        makeSearchResult());

    SearchTimerPreviewResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(previewResult);

    assert(json.find("\"searchTimer\":{") != std::string::npos);
    assert(json.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"searchtimer-1\"") != std::string::npos);
    assert(json.find("\"name\":\"Terra X Suche\"") != std::string::npos);
    assert(json.find("\"query\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"state\":\"active\"") != std::string::npos);
    assert(json.find("\"previewEngine\":\"suite-epg-cache\"") != std::string::npos);
    assert(json.find("\"epgInput\":{") != std::string::npos);
    assert(json.find("\"status\":\"ready\"") != std::string::npos);
    assert(json.find("\"available\":true") != std::string::npos);
    assert(json.find("\"warnings\":[]") != std::string::npos);
    assert(json.find("\"statistics\":{") != std::string::npos);
    assert(json.find("\"channelCount\":1") != std::string::npos);
    assert(json.find("\"nextStartTime\":\"1000\"") != std::string::npos);
    assert(json.find("\"latestStartTime\":\"1000\"") != std::string::npos);
    assert(json.find("\"preview\":{") != std::string::npos);
    assert(json.find("\"totalCount\":1") != std::string::npos);
    assert(json.find("\"returnedCount\":1") != std::string::npos);
    assert(json.find("\"event\":{\"id\":\"event-1\"") != std::string::npos);
    assert(json.find("\"matchedFields\":[\"title\"]") != std::string::npos);
}

static void test_warming_epg_input_is_explicit()
{
    SearchTimerPreviewResult previewResult(
        makeSearchTimer(),
        EpgSearchResult::empty(10, 0),
        "warming",
        false,
        {"SearchTimer preview EPG input is warming."});

    SearchTimerPreviewResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(previewResult);

    assert(json.find("\"previewEngine\":\"suite-epg-cache\"") != std::string::npos);
    assert(json.find("\"epgInput\":{") != std::string::npos);
    assert(json.find("\"status\":\"warming\"") != std::string::npos);
    assert(json.find("\"available\":false") != std::string::npos);
    assert(json.find("SearchTimer preview EPG input is warming") != std::string::npos);
    assert(json.find("\"totalCount\":0") != std::string::npos);
    assert(json.find("\"returnedCount\":0") != std::string::npos);
}

static void test_custom_preview_engine_is_serialized()
{
    SearchTimerPreviewResult previewResult(
        makeSearchTimer(),
        EpgSearchResult::empty(10, 0),
        "ready",
        true,
        {},
        "native-epgsearch");

    SearchTimerPreviewResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(previewResult);

    assert(json.find("\"previewEngine\":\"native-epgsearch\"") != std::string::npos);
}

int main()
{
    test_default_result_reports_ready_epg_input();
    test_warming_epg_input_is_explicit();
    test_custom_preview_engine_is_serialized();

    std::cout
        << "test_search_timer_preview_result_json_serializer passed"
        << std::endl;

    return 0;
}
