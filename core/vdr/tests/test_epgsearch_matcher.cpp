#include "EpgSearchMatcher.h"
#include "EpgSearchQuery.h"
#include "VdrEvent.h"

#include <cassert>
#include <iostream>

namespace {
VdrEvent makeEvent()
{
    VdrEvent event;
    event.id = "event-1";
    event.title = "Terra X";
    event.subtitle = "Faszination Erde";
    event.description = "Dokumentation über Natur und Geschichte";
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

    std::cout
        << "test_epgsearch_matcher passed"
        << std::endl;

    return 0;
}
