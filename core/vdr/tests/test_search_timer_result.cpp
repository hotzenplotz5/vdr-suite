#include "SearchTimerResult.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    SearchTimerResult emptyResult = SearchTimerResult::empty(25, 10);
    assert(emptyResult.empty());
    assert(emptyResult.returnedCount() == 0);
    assert(emptyResult.totalCount() == 0);
    assert(emptyResult.limit() == 25);
    assert(emptyResult.offset() == 10);

    SearchTimer first = SearchTimer::create(
        SearchTimerId::fromBackendNativeId("livingroom", "1"),
        "Terra X",
        "Terra X",
        SearchTimerState::Active);

    SearchTimer second = SearchTimer::create(
        SearchTimerId::fromBackendNativeId("livingroom", "2"),
        "Tatort",
        "Tatort",
        SearchTimerState::Inactive);

    std::vector<SearchTimer> items;
    items.push_back(first);
    items.push_back(second);

    SearchTimerResult result = SearchTimerResult::from(
        items,
        7,
        2,
        4);

    assert(!result.empty());
    assert(result.returnedCount() == 2);
    assert(result.totalCount() == 7);
    assert(result.limit() == 2);
    assert(result.offset() == 4);
    assert(result.items().at(0).name() == "Terra X");
    assert(result.items().at(0).backendId() == "livingroom");
    assert(result.items().at(0).backendNativeId() == "1");
    assert(result.items().at(1).name() == "Tatort");
    assert(result.items().at(1).isInactive());

    std::cout << "test_search_timer_result passed" << std::endl;
    return 0;
}