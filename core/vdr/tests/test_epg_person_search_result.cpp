#include "EpgPersonSearchResult.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

VdrEvent makeEvent()
{
    VdrEvent event;

    event.id = "event-1";
    event.channelId = "channel-1";
    event.title = "Movie Night";
    event.subtitle = "Special";
    event.description = "A movie with cast metadata.";
    event.startTime = "2026-06-20T20:15:00";
    event.endTime = "2026-06-20T22:00:00";
    event.durationSeconds = 6300;
    event.parentalRating = 12;

    return event;
}

Person makePerson()
{
    return Person::withCharacterName(
        ContentClassificationSource::Epg,
        PersonRole::Actor,
        "Jim Carrey",
        "jim-carrey",
        "Ace Ventura");
}

}

int main()
{
    EpgPersonSearchResult emptyResult =
        EpgPersonSearchResult::empty(
            10,
            2);

    assert(emptyResult.empty());
    assert(emptyResult.totalCount() == 0);
    assert(emptyResult.returnedCount() == 0);
    assert(emptyResult.limit() == 10);
    assert(emptyResult.offset() == 2);

    EpgPersonSearchMatch match(
        makeEvent(),
        "backend-a",
        makePerson());

    assert(match.event().id == "event-1");
    assert(match.event().title == "Movie Night");
    assert(match.backendId() == "backend-a");
    assert(match.hasBackendId());
    assert(match.person().originalName() == "Jim Carrey");
    assert(match.person().characterName() == "Ace Ventura");

    std::vector<EpgPersonSearchMatch> matches;
    matches.push_back(match);

    EpgPersonSearchResult result =
        EpgPersonSearchResult::from(
            matches,
            3,
            1,
            1);

    assert(!result.empty());
    assert(result.totalCount() == 3);
    assert(result.returnedCount() == 1);
    assert(result.limit() == 1);
    assert(result.offset() == 1);
    assert(result.matches().at(0).event().channelId == "channel-1");
    assert(result.matches().at(0).person().normalizedName() == "jim-carrey");

    std::cout << "test_epg_person_search_result passed" << std::endl;
    return 0;
}
