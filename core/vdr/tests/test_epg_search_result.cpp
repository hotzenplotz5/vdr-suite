#include "EpgSearchResult.h"

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
    EpgSearchResult empty =
        EpgSearchResult::empty(
            50,
            100);

    assert(empty.empty());
    assert(empty.matches().empty());
    assert(empty.totalCount() == 0);
    assert(empty.returnedCount() == 0);
    assert(empty.limit() == 50);
    assert(empty.offset() == 100);

    EpgSearchMatch plain =
        EpgSearchMatch::fromEvent(
            makeEvent(
                "event-1",
                "Tatort"));

    assert(plain.event().id == "event-1");
    assert(plain.event().title == "Tatort");
    assert(!plain.hasBackendId());
    assert(!plain.hasMatchedFields());

    EpgSearchMatch backendMatch(
        makeEvent(
            "event-2",
            "Tagesschau"),
        "living-room",
        {"title", "description"});

    assert(backendMatch.event().id == "event-2");
    assert(backendMatch.hasBackendId());
    assert(backendMatch.backendId() == "living-room");
    assert(backendMatch.hasMatchedFields());
    assert(backendMatch.matchedFields().size() == 2);
    assert(backendMatch.matchedFields().at(0) == "title");
    assert(backendMatch.matchedFields().at(1) == "description");

    std::vector<EpgSearchMatch> matches;
    matches.push_back(plain);
    matches.push_back(backendMatch);

    EpgSearchResult result(
        matches,
        973,
        2,
        10);

    assert(!result.empty());
    assert(result.matches().size() == 2);
    assert(result.returnedCount() == 2);
    assert(result.totalCount() == 973);
    assert(result.limit() == 2);
    assert(result.offset() == 10);
    assert(result.matches().at(0).event().title == "Tatort");
    assert(result.matches().at(1).event().title == "Tagesschau");
    assert(result.matches().at(1).backendId() == "living-room");

    std::cout
        << "test_epg_search_result passed"
        << std::endl;

    return 0;
}
