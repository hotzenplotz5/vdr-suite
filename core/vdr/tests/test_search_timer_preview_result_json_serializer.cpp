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

int main()
{
    SearchTimer searchTimer = SearchTimer::create(
        SearchTimerId::fromBackendNativeId("home-vdr", "searchtimer-1"),
        "Terra X Suche",
        "Terra X",
        SearchTimerState::Active);

    EpgSearchResult searchResult(
        {
            EpgSearchMatch(
                makeEvent(),
                "home-vdr",
                {"title"})
        },
        1,
        10,
        0);

    SearchTimerPreviewResult previewResult(
        searchTimer,
        searchResult);

    SearchTimerPreviewResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(previewResult);

    assert(json.find("\"searchTimer\":{") != std::string::npos);
    assert(json.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"searchtimer-1\"") != std::string::npos);
    assert(json.find("\"name\":\"Terra X Suche\"") != std::string::npos);
    assert(json.find("\"query\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"state\":\"active\"") != std::string::npos);
    assert(json.find("\"preview\":{") != std::string::npos);
    assert(json.find("\"totalCount\":1") != std::string::npos);
    assert(json.find("\"returnedCount\":1") != std::string::npos);
    assert(json.find("\"eventId\":\"event-1\"") != std::string::npos);
    assert(json.find("\"matchedFields\":[\"title\"]") != std::string::npos);

    std::cout
        << "test_search_timer_preview_result_json_serializer passed"
        << std::endl;

    return 0;
}
