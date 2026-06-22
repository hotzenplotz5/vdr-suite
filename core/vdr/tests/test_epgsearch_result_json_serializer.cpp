#include "EpgSearchMatch.h"
#include "EpgSearchResult.h"
#include "EpgSearchResultJsonSerializer.h"
#include "VdrEvent.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {
bool contains(
    const std::string& value,
    const std::string& needle)
{
    return value.find(needle) != std::string::npos;
}

VdrEvent makeEvent()
{
    VdrEvent event;
    event.id = "event-1";
    event.channelId = "C-1-1051-10301";
    event.title = "Terra X";
    event.subtitle = "Faszination Erde";
    event.description = "Dokumentation mit \"Zitat\"";
    event.startTime = "2026-06-22 20:15:00";
    event.endTime = "2026-06-22 21:15:00";
    event.durationSeconds = 3600;
    event.contentDescriptors = {
        "0x10",
        "0x11"
    };
    event.parentalRating = 6;
    return event;
}
}

int main()
{
    const EpgSearchResultJsonSerializer serializer;

    const std::string emptyJson =
        serializer.serialize(
            EpgSearchResult::empty(10, 20));

    assert(contains(emptyJson, "\"totalCount\":0"));
    assert(contains(emptyJson, "\"returnedCount\":0"));
    assert(contains(emptyJson, "\"limit\":10"));
    assert(contains(emptyJson, "\"offset\":20"));
    assert(contains(emptyJson, "\"matches\":[]"));

    const EpgSearchResult result(
        {
            EpgSearchMatch(
                makeEvent(),
                "home-vdr",
                {"title", "description"})
        },
        1,
        50,
        0);

    const std::string json =
        serializer.serialize(result);

    assert(contains(json, "\"totalCount\":1"));
    assert(contains(json, "\"returnedCount\":1"));
    assert(contains(json, "\"backendId\":\"home-vdr\""));
    assert(contains(json, "\"matchedFields\":[\"title\",\"description\"]"));
    assert(contains(json, "\"id\":\"event-1\""));
    assert(contains(json, "\"channelId\":\"C-1-1051-10301\""));
    assert(contains(json, "\"title\":\"Terra X\""));
    assert(contains(json, "\"description\":\"Dokumentation mit \\\"Zitat\\\"\""));
    assert(contains(json, "\"durationSeconds\":3600"));
    assert(contains(json, "\"contentDescriptors\":[\"0x10\",\"0x11\"]"));
    assert(contains(json, "\"parentalRating\":6"));

    std::cout
        << "test_epgsearch_result_json_serializer passed"
        << std::endl;

    return 0;
}
