#include "EpgSearchService.h"

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
    const std::string& startTime,
    int durationSeconds)
{
    VdrEvent event;
    event.id = id;
    event.channelId = channelId;
    event.title = title;
    event.subtitle = subtitle;
    event.description = description;
    event.startTime = startTime;
    event.endTime = startTime;
    event.durationSeconds = durationSeconds;
    return event;
}

int main()
{
    std::vector<VdrEvent> events;
    events.push_back(
        makeEvent(
            "event-1",
            "channel-1",
            "Tagesschau",
            "20 Uhr",
            "Nachrichten",
            "1780000000",
            900));
    events.push_back(
        makeEvent(
            "event-2",
            "channel-1",
            "Tatort",
            "Borowski",
            "Krimi aus Kiel",
            "1780001000",
            5400));
    events.push_back(
        makeEvent(
            "event-3",
            "channel-2",
            "Filmnacht",
            "Tatort Spezial",
            "Lange Kriminacht",
            "1780002000",
            7200));

    EpgSearchService service;

    EpgSearchResult all =
        service.search(
            events,
            EpgSearchRequest::all());

    assert(all.totalCount() == 3);
    assert(all.returnedCount() == 3);
    assert(all.limit() == 0);
    assert(all.offset() == 0);

    EpgSearchResult text =
        service.search(
            events,
            EpgSearchRequest::text(
                "tatort",
                10,
                0));

    assert(text.totalCount() == 2);
    assert(text.returnedCount() == 2);
    assert(text.matches().at(0).event().id == "event-2");
    assert(text.matches().at(0).matchedFields().at(0) == "title");
    assert(text.matches().at(1).event().id == "event-3");
    assert(text.matches().at(1).matchedFields().at(0) == "subtitle");

    EpgSearchResult paged =
        service.search(
            events,
            EpgSearchRequest::text(
                "tatort",
                1,
                1));

    assert(paged.totalCount() == 2);
    assert(paged.returnedCount() == 1);
    assert(paged.limit() == 1);
    assert(paged.offset() == 1);
    assert(paged.matches().at(0).event().id == "event-3");

    EpgSearchResult channelWindow =
        service.search(
            events,
            EpgSearchRequest::window(
                "",
                "channel-1",
                1780000500,
                1000,
                10,
                0));

    assert(channelWindow.totalCount() == 1);
    assert(channelWindow.returnedCount() == 1);
    assert(channelWindow.matches().at(0).event().id == "event-2");

    EpgSearchResult backend =
        service.search(
            events,
            EpgSearchRequest::backendWindow(
                "living-room",
                "tatort",
                "",
                -1,
                0,
                10,
                0));

    assert(backend.totalCount() == 2);
    assert(backend.matches().at(0).backendId() == "living-room");
    assert(backend.matches().at(1).backendId() == "living-room");

    EpgSearchResult sorted =
        service.search(
            events,
            EpgSearchRequest::sorted(
                "",
                "",
                "",
                -1,
                0,
                0,
                0,
                EpgSearchSortField::Duration,
                EpgSearchSortOrder::Descending));

    assert(sorted.totalCount() == 3);
    assert(sorted.matches().at(0).event().id == "event-3");
    assert(sorted.matches().at(1).event().id == "event-2");
    assert(sorted.matches().at(2).event().id == "event-1");

    EpgSearchResult empty =
        service.search(
            events,
            EpgSearchRequest::text(
                "not-found",
                10,
                0));

    assert(empty.empty());
    assert(empty.totalCount() == 0);
    assert(empty.returnedCount() == 0);

    std::cout
        << "test_epg_search_service passed"
        << std::endl;

    return 0;
}
