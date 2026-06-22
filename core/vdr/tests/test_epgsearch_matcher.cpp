#include "EpgSearchMatcher.h"
#include "EpgSearchQuery.h"
#include "VdrEvent.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace {
VdrEvent makeEvent()
{
    VdrEvent event;
    event.id = "event-1";
    event.channelId = "C-1-1051-10301";
    event.title = "Terra X";
    event.subtitle = "Faszination Erde";
    event.description = "Dokumentation über Natur und Geschichte";
    event.durationSeconds = 3600;
    event.contentDescriptors = {
        "0x10",
        "0x11"
    };
    return event;
}
}

int main()
{
    const EpgSearchMatcher matcher;
    const VdrEvent event =
        makeEvent();

    assert(matcher.matches(
        event,
        EpgSearchQuery::all()));

    assert(matcher.matches(
        event,
        EpgSearchQuery::byText("terra")));

    assert(!matcher.matches(
        event,
        EpgSearchQuery::byText("terra")
            .withMatchCase(true)));

    assert(matcher.matches(
        event,
        EpgSearchQuery::byText("Terra")
            .withMatchCase(true)));

    assert(matcher.matches(
        event,
        EpgSearchQuery::byText("Faszination")
            .searchInSubtitle(true)));

    assert(!matcher.matches(
        event,
        EpgSearchQuery::byText("Faszination")
            .searchInTitle(true)));

    assert(matcher.matches(
        event,
        EpgSearchQuery::byText("geschichte")
            .searchInDescription(true)));

    assert(!matcher.matches(
        event,
        EpgSearchQuery::byText("Krimi")
            .searchInTitle(true)
            .searchInSubtitle(true)
            .searchInDescription(true)));

    assert(!matcher.matches(
        event,
        EpgSearchQuery::byText("Terra")
            .searchInTitle(false)
            .searchInSubtitle(false)
            .searchInDescription(false)));

    assert(matcher.matches(
        event,
        EpgSearchQuery::all()
            .withChannelInterval(
                "C-1-0000-00000",
                "C-9-9999-99999")));

    assert(!matcher.matches(
        event,
        EpgSearchQuery::all()
            .withChannelInterval(
                "S-1-0000-00000",
                "S-9-9999-99999")));

    assert(matcher.matches(
        event,
        EpgSearchQuery::all()
            .withDurationWindow(45, 90)));

    assert(!matcher.matches(
        event,
        EpgSearchQuery::all()
            .withDurationWindow(61, 90)));

    assert(matcher.matches(
        event,
        EpgSearchQuery::all()
            .withContentDescriptors("0x10,0x11")));

    assert(!matcher.matches(
        event,
        EpgSearchQuery::all()
            .withContentDescriptors("0x10,0x20")));

    std::cout
        << "test_epgsearch_matcher passed"
        << std::endl;

    return 0;
}
