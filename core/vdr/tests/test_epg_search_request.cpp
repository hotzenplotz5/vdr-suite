#include "EpgSearchRequest.h"

#include <cassert>
#include <iostream>

int main()
{
    EpgSearchRequest all =
        EpgSearchRequest::all();

    assert(!all.hasQueryText());
    assert(all.queryText().empty());
    assert(!all.hasBackendId());
    assert(!all.hasChannelId());
    assert(!all.hasFrom());
    assert(!all.hasTimespan());
    assert(!all.hasLimit());
    assert(all.limit() == 0);
    assert(all.offset() == 0);
    assert(all.searchTitle());
    assert(all.searchSubtitle());
    assert(all.searchDescription());
    assert(all.hasSearchField());
    assert(!all.hasSort());
    assert(!all.hasSearchMode());
    assert(!all.hasFuzzyTolerance());

    EpgSearchRequest text =
        EpgSearchRequest::text(
            "Tatort",
            25,
            50);

    assert(text.hasQueryText());
    assert(text.queryText() == "Tatort");
    assert(text.hasLimit());
    assert(text.limit() == 25);
    assert(text.offset() == 50);
    assert(!text.hasBackendId());
    assert(!text.hasChannelId());

    EpgSearchRequest window =
        EpgSearchRequest::window(
            "Nachrichten",
            "S19.2E-1-1011-11110",
            1780000000,
            7200,
            10,
            5);

    assert(window.queryText() == "Nachrichten");
    assert(window.hasChannelId());
    assert(window.channelId() == "S19.2E-1-1011-11110");
    assert(window.hasFrom());
    assert(window.from() == 1780000000);
    assert(window.hasTimespan());
    assert(window.timespan() == 7200);
    assert(window.limit() == 10);
    assert(window.offset() == 5);

    EpgSearchRequest backendWindow =
        EpgSearchRequest::backendWindow(
            "living-room",
            "Film",
            "channel-1",
            1780001000,
            3600,
            20,
            0);

    assert(backendWindow.hasBackendId());
    assert(backendWindow.backendId() == "living-room");
    assert(backendWindow.channelId() == "channel-1");
    assert(backendWindow.queryText() == "Film");

    EpgSearchRequest sorted =
        EpgSearchRequest::sorted(
            "living-room",
            "Krimi",
            "channel-2",
            1780002000,
            1800,
            15,
            3,
            EpgSearchSortField::StartTime,
            EpgSearchSortOrder::Descending);

    assert(sorted.hasSort());
    assert(sorted.sortField() == EpgSearchSortField::StartTime);
    assert(sorted.sortOrder() == EpgSearchSortOrder::Descending);
    assert(sorted.sortDescending());
    assert(EpgSearchSortField::Title != EpgSearchSortField::None);
    assert(EpgSearchSortField::Duration != EpgSearchSortField::None);

    sorted.setSearchFields(
        true,
        false,
        true);

    assert(sorted.searchTitle());
    assert(!sorted.searchSubtitle());
    assert(sorted.searchDescription());
    assert(sorted.hasSearchField());

    sorted.setSearchFields(
        false,
        false,
        false);

    assert(!sorted.hasSearchField());

    sorted.setSearchMode(EpgSearchMode::Fuzzy);
    sorted.setFuzzyTolerance(2);

    assert(sorted.hasSearchMode());
    assert(sorted.searchMode() == EpgSearchMode::Fuzzy);
    assert(sorted.hasFuzzyTolerance());
    assert(sorted.fuzzyTolerance() == 2);

    std::cout
        << "test_epg_search_request passed"
        << std::endl;

    return 0;
}
