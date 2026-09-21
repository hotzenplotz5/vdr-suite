#include "SearchTimerQuery.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerQuery emptyQuery = SearchTimerQuery::all();
    assert(emptyQuery.isEmpty());
    assert(emptyQuery.matchesAll());
    assert(!emptyQuery.hasBackend());
    assert(!emptyQuery.hasState());
    assert(!emptyQuery.hasText());
    assert(!emptyQuery.hasLimit());

    SearchTimerQuery limitedQuery = SearchTimerQuery::limited(10, 5);
    assert(!limitedQuery.isEmpty());
    assert(!limitedQuery.matchesAll());
    assert(limitedQuery.hasLimit());
    assert(limitedQuery.limit() == 10);
    assert(limitedQuery.offset() == 5);

    SearchTimerQuery backendQuery = SearchTimerQuery::byBackend("livingroom");
    assert(backendQuery.hasBackend());
    assert(backendQuery.backendId() == "livingroom");

    SearchTimerQuery stateQuery = SearchTimerQuery::byState(SearchTimerState::Active);
    assert(stateQuery.hasState());
    assert(stateQuery.state() == SearchTimerState::Active);

    SearchTimerQuery textQuery = SearchTimerQuery::byText("Terra X");
    assert(textQuery.hasText());
    assert(textQuery.text() == "Terra X");

    SearchTimerQuery combinedQuery = SearchTimerQuery::all()
        .withBackend("livingroom")
        .withState(SearchTimerState::Inactive)
        .withText("Tatort")
        .withLimit(25, 10);

    assert(!combinedQuery.isEmpty());
    assert(combinedQuery.hasBackend());
    assert(combinedQuery.backendId() == "livingroom");
    assert(combinedQuery.hasState());
    assert(combinedQuery.state() == SearchTimerState::Inactive);
    assert(combinedQuery.hasText());
    assert(combinedQuery.text() == "Tatort");
    assert(combinedQuery.hasLimit());
    assert(combinedQuery.limit() == 25);
    assert(combinedQuery.offset() == 10);

    std::cout << "test_search_timer_query passed" << std::endl;
    return 0;
}