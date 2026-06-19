#include "EpgSearchMatcher.h"

#include <cassert>
#include <iostream>

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
    event.endTime = "1780003600";
    event.durationSeconds = 3600;
    return event;
}

int main()
{
    EpgSearchMatcher matcher;

    VdrEvent tatort =
        makeEvent(
            "event-1",
            "channel-1",
            "Tatort",
            "Borowski und das Meer",
            "Ein Kriminalfall an der Kueste",
            "1780000000");

    assert(matcher.matches(
        tatort,
        EpgSearchRequest::all()));

    assert(matcher.matches(
        tatort,
        EpgSearchRequest::text(
            "Tatort",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        EpgSearchRequest::text(
            "tator",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        EpgSearchRequest::text(
            "TATORT",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        EpgSearchRequest::text(
            "Borowski",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        EpgSearchRequest::text(
            "kriminalfall",
            10,
            0)));

    assert(!matcher.matches(
        tatort,
        EpgSearchRequest::text(
            "Tagesschau",
            10,
            0)));

    EpgSearchRequest titleOnly =
        EpgSearchRequest::text(
            "Borowski",
            10,
            0);
    titleOnly.setSearchFields(
        true,
        false,
        false);

    assert(!matcher.matches(
        tatort,
        titleOnly));

    EpgSearchRequest subtitleOnly =
        EpgSearchRequest::text(
            "Borowski",
            10,
            0);
    subtitleOnly.setSearchFields(
        false,
        true,
        false);

    assert(matcher.matches(
        tatort,
        subtitleOnly));

    EpgSearchRequest noSearchFields =
        EpgSearchRequest::text(
            "Tatort",
            10,
            0);
    noSearchFields.setSearchFields(
        false,
        false,
        false);

    assert(!matcher.matches(
        tatort,
        noSearchFields));

    assert(matcher.matches(
        tatort,
        EpgSearchRequest::window(
            "Tatort",
            "channel-1",
            1779999000,
            7200,
            10,
            0)));

    assert(!matcher.matches(
        tatort,
        EpgSearchRequest::window(
            "Tatort",
            "channel-2",
            1779999000,
            7200,
            10,
            0)));

    assert(!matcher.matches(
        tatort,
        EpgSearchRequest::window(
            "Tatort",
            "channel-1",
            1780001000,
            7200,
            10,
            0)));

    assert(!matcher.matches(
        tatort,
        EpgSearchRequest::window(
            "Tatort",
            "channel-1",
            1779990000,
            100,
            10,
            0)));

    VdrEvent textualTime =
        makeEvent(
            "event-2",
            "channel-1",
            "Tatort",
            "",
            "",
            "2026-06-01T20:00:00");

    assert(matcher.matches(
        textualTime,
        EpgSearchRequest::window(
            "Tatort",
            "channel-1",
            1780001000,
            7200,
            10,
            0)));

    std::cout
        << "test_epg_search_matcher passed"
        << std::endl;

    return 0;
}
