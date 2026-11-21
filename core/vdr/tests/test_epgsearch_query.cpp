#include "EpgSearchQuery.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    const EpgSearchQuery empty =
        EpgSearchQuery::all();

    assert(empty.isEmpty());
    assert(empty.matchesAll());
    assert(!empty.hasText());
    assert(!empty.hasBackend());

    const EpgSearchQuery textQuery =
        EpgSearchQuery::byText("Tatort");

    assert(!textQuery.isEmpty());
    assert(textQuery.hasText());
    assert(textQuery.text() == "Tatort");

    EpgSearchQuery query =
        EpgSearchQuery::byText("Terra X")
            .withBackend("home-vdr")
            .withMode(EpgSearchMode::Fuzzy)
            .withFuzzyTolerance(2)
            .searchInTitle(true)
            .searchInSubtitle(true)
            .searchInDescription(false)
            .withMatchCase(true)
            .withChannelInterval(
                "C-1-1051-10301",
                "C-1-1079-11110")
            .withTimeWindow(2015, 2230)
            .withDurationWindow(45, 120)
            .withDayOfWeek(62)
            .withExtendedEpgInfo(
                std::vector<std::string>{
                    "Kategorie=Dokumentation",
                    "Genre=Wissen"})
            .withContentDescriptors("0x10,0x11")
            .withFavoritesOnly(true);

    assert(!query.isEmpty());
    assert(!query.matchesAll());

    assert(query.hasBackend());
    assert(query.backendId() == "home-vdr");

    assert(query.hasText());
    assert(query.text() == "Terra X");

    assert(query.hasMode());
    assert(query.mode() == EpgSearchMode::Fuzzy);

    assert(query.hasFuzzyTolerance());
    assert(query.fuzzyTolerance() == 2);

    assert(query.hasFieldSelection());
    assert(query.useTitle());
    assert(query.useSubtitle());
    assert(!query.useDescription());

    assert(query.hasMatchCase());
    assert(query.matchCase());

    assert(query.hasChannelScope());
    assert(query.channelScope() == EpgSearchChannelScope::Interval);
    assert(query.channelMin() == "C-1-1051-10301");
    assert(query.channelMax() == "C-1-1079-11110");

    assert(query.hasTimeWindow());
    assert(query.startTime() == 2015);
    assert(query.stopTime() == 2230);

    assert(query.hasDurationWindow());
    assert(query.durationMinMinutes() == 45);
    assert(query.durationMaxMinutes() == 120);

    assert(query.hasDayOfWeek());
    assert(query.dayOfWeek() == 62);

    assert(query.hasExtendedEpgInfo());
    assert(query.extendedEpgInfo().size() == 2);
    assert(query.extendedEpgInfo().at(0) == "Kategorie=Dokumentation");

    assert(query.hasContentDescriptors());
    assert(query.contentDescriptors() == "0x10,0x11");

    assert(query.hasFavoritesOnly());
    assert(query.favoritesOnly());

    const EpgSearchQuery groupQuery =
        EpgSearchQuery::byText("Sport")
            .withChannelGroup("Sports")
            .withFavoritesOnly(false);

    assert(groupQuery.hasChannelScope());
    assert(groupQuery.channelScope() == EpgSearchChannelScope::Group);
    assert(groupQuery.channelGroup() == "Sports");
    assert(groupQuery.hasFavoritesOnly());
    assert(!groupQuery.favoritesOnly());

    const EpgSearchQuery ftaQuery =
        EpgSearchQuery::byText("News")
            .withFreeToAirOnly();

    assert(ftaQuery.hasChannelScope());
    assert(ftaQuery.channelScope() == EpgSearchChannelScope::FreeToAir);

    std::cout
        << "test_epgsearch_query passed"
        << std::endl;

    return 0;
}
