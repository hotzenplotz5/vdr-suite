#include "EpgSearchResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

static VdrEvent makeEvent(
    const std::string& id,
    const std::string& title)
{
    VdrEvent event;
    event.id = id;
    event.channelId = "channel-1";
    event.title = title;
    event.subtitle = "Subtitle";
    event.description = "Description";
    event.startTime = "1780000000";
    event.endTime = "1780003600";
    event.durationSeconds = 3600;
    return event;
}

int main()
{
    std::vector<EpgSearchMatch> matches;

    matches.push_back(
        EpgSearchMatch(
            makeEvent(
                "event-1",
                "Tatort"),
            "living-room",
            {"title", "description"}));

    matches.push_back(
        EpgSearchMatch(
            makeEvent(
                "event-2",
                "Tagesschau"),
            "ferienhaus",
            {"subtitle"}));

    EpgSearchResult result(
        matches,
        973,
        2,
        10);

    EpgSearchResultJsonSerializer serializer;
    std::string json =
        serializer.serialize(result);

    assert(json.find("\"totalCount\":973") != std::string::npos);
    assert(json.find("\"returnedCount\":2") != std::string::npos);
    assert(json.find("\"limit\":2") != std::string::npos);
    assert(json.find("\"offset\":10") != std::string::npos);
    assert(json.find("\"results\":[") != std::string::npos);
    assert(json.find("\"eventId\":\"event-1\"") != std::string::npos);
    assert(json.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(json.find("\"channelId\":\"channel-1\"") != std::string::npos);
    assert(json.find("\"title\":\"Tatort\"") != std::string::npos);
    assert(json.find("\"durationSeconds\":3600") != std::string::npos);
    assert(json.find("\"matchedFields\":[\"title\",\"description\"]") != std::string::npos);
    assert(json.find("\"eventId\":\"event-2\"") != std::string::npos);
    assert(json.find("\"backendId\":\"ferienhaus\"") != std::string::npos);
    assert(json.find("\"matchedFields\":[\"subtitle\"]") != std::string::npos);

    VdrEvent escaped =
        makeEvent(
            "event-3",
            "Film \"Quote\"");
    escaped.description = "Line 1\nLine 2\\Path";

    EpgSearchResult escapedResult(
        {EpgSearchMatch(
            escaped,
            "",
            {})},
        1,
        1,
        0);

    std::string escapedJson =
        serializer.serialize(escapedResult);

    assert(escapedJson.find("Film \\\"Quote\\\"") != std::string::npos);
    assert(escapedJson.find("Line 1\\nLine 2\\\\Path") != std::string::npos);
    assert(escapedJson.find("\"matchedFields\":[]") != std::string::npos);

    EpgSearchResult empty =
        EpgSearchResult::empty(
            50,
            100);

    std::string emptyJson =
        serializer.serialize(empty);

    assert(emptyJson == "{\"totalCount\":0,\"returnedCount\":0,\"limit\":50,\"offset\":100,\"results\":[]}");

    std::cout
        << "test_epg_search_result_json_serializer passed"
        << std::endl;

    return 0;
}
