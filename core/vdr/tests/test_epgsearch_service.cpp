#include "EpgSearchQuery.h"
#include "EpgSearchService.h"
#include "VdrEvent.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace {
VdrEvent makeEvent(
    const std::string& id,
    const std::string& channelId,
    const std::string& title,
    const std::string& subtitle,
    const std::string& description,
    int durationSeconds = 0)
{
    VdrEvent event;
    event.id = id;
    event.channelId = channelId;
    event.title = title;
    event.subtitle = subtitle;
    event.description = description;
    event.durationSeconds = durationSeconds;
    return event;
}
}

int main()
{
    const std::vector<VdrEvent> events = {
        makeEvent(
            "event-1",
            "channel-1",
            "Terra X",
            "Faszination Erde",
            "Dokumentation über Natur und Geschichte",
            3600),
        makeEvent(
            "event-2",
            "channel-1",
            "Tagesschau",
            "Nachrichten",
            "Aktuelle Themen des Tages",
            900),
        makeEvent(
            "event-3",
            "channel-2",
            "Tatort",
            "Borowski",
            "Krimi aus Kiel",
            5400)
    };

    const EpgSearchService service;

    const EpgSearchResult allResult =
        service.search(
            events,
            EpgSearchQuery::all());

    assert(allResult.totalCount() == 3);
    assert(allResult.returnedCount() == 3);
    assert(!allResult.empty());

    const EpgSearchResult titleResult =
        service.search(
            events,
            EpgSearchQuery::byText("Terra")
                .searchInTitle(true));

    assert(titleResult.totalCount() == 1);
    assert(titleResult.returnedCount() == 1);
    assert(titleResult.matches().at(0).event().id == "event-1");

    const EpgSearchResult descriptionResult =
        service.search(
            events,
            EpgSearchQuery::byText("kiel")
                .searchInDescription(true));

    assert(descriptionResult.totalCount() == 1);
    assert(descriptionResult.matches().at(0).event().id == "event-3");

    const EpgSearchResult caseSensitiveMiss =
        service.search(
            events,
            EpgSearchQuery::byText("terra")
                .withMatchCase(true)
                .searchInTitle(true));

    assert(caseSensitiveMiss.empty());
    assert(caseSensitiveMiss.totalCount() == 0);

    const EpgSearchResult caseInsensitiveHit =
        service.search(
            events,
            EpgSearchQuery::byText("terra")
                .searchInTitle(true));

    assert(caseInsensitiveHit.totalCount() == 1);

    const EpgSearchResult backendScopedResult =
        service.search(
            events,
            EpgSearchQuery::byText("tatort")
                .withBackend("living-room"));

    assert(backendScopedResult.totalCount() == 1);
    assert(backendScopedResult.returnedCount() == 1);

    const EpgSearchResult channelResult =
        service.search(
            events,
            EpgSearchQuery::all()
                .withChannelInterval(
                    "channel-2",
                    "channel-2"));

    assert(channelResult.totalCount() == 1);
    assert(channelResult.matches().at(0).event().id == "event-3");

    const EpgSearchResult durationResult =
        service.search(
            events,
            EpgSearchQuery::all()
                .withDurationWindow(50, 70));

    assert(durationResult.totalCount() == 1);
    assert(durationResult.matches().at(0).event().id == "event-1");

    std::cout
        << "test_epgsearch_service passed"
        << std::endl;

    return 0;
}
