#include "SearchTimerService.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

SearchTimer makeTimer(
    const std::string& backendId,
    const std::string& nativeId,
    const std::string& name,
    const std::string& query,
    SearchTimerState state)
{
    return SearchTimer::create(
        SearchTimerId::fromBackendNativeId(
            backendId,
            nativeId),
        name,
        query,
        state);
}

} // namespace

int main()
{
    std::vector<SearchTimer> timers;
    timers.push_back(makeTimer("livingroom", "1", "Terra X", "Terra X", SearchTimerState::Active));
    timers.push_back(makeTimer("livingroom", "2", "Tatort", "Tatort", SearchTimerState::Inactive));
    timers.push_back(makeTimer("bedroom", "1", "Tagesschau", "Tagesschau", SearchTimerState::Active));
    timers.push_back(makeTimer("bedroom", "2", "Science", "Universum", SearchTimerState::Inactive));

    SearchTimerService service;

    SearchTimerResult all = service.list(
        timers,
        SearchTimerQuery::all());
    assert(all.returnedCount() == 4);
    assert(all.totalCount() == 4);

    SearchTimerResult livingroom = service.list(
        timers,
        SearchTimerQuery::byBackend("livingroom"));
    assert(livingroom.returnedCount() == 2);
    assert(livingroom.totalCount() == 2);
    assert(livingroom.items().at(0).backendId() == "livingroom");
    assert(livingroom.items().at(1).backendId() == "livingroom");

    SearchTimerResult active = service.list(
        timers,
        SearchTimerQuery::byState(SearchTimerState::Active));
    assert(active.returnedCount() == 2);
    assert(active.totalCount() == 2);
    assert(active.items().at(0).isActive());
    assert(active.items().at(1).isActive());

    SearchTimerResult inactive = service.list(
        timers,
        SearchTimerQuery::byState(SearchTimerState::Inactive));
    assert(inactive.returnedCount() == 2);
    assert(inactive.totalCount() == 2);
    assert(inactive.items().at(0).isInactive());
    assert(inactive.items().at(1).isInactive());

    SearchTimerResult textByName = service.list(
        timers,
        SearchTimerQuery::byText("Science"));
    assert(textByName.returnedCount() == 1);
    assert(textByName.items().at(0).name() == "Science");

    SearchTimerResult textByQuery = service.list(
        timers,
        SearchTimerQuery::byText("Universum"));
    assert(textByQuery.returnedCount() == 1);
    assert(textByQuery.items().at(0).query() == "Universum");

    SearchTimerResult combined = service.list(
        timers,
        SearchTimerQuery::all()
            .withBackend("bedroom")
            .withState(SearchTimerState::Active));
    assert(combined.returnedCount() == 1);
    assert(combined.totalCount() == 1);
    assert(combined.items().at(0).name() == "Tagesschau");

    SearchTimerResult page = service.list(
        timers,
        SearchTimerQuery::limited(2, 1));
    assert(page.returnedCount() == 2);
    assert(page.totalCount() == 4);
    assert(page.limit() == 2);
    assert(page.offset() == 1);
    assert(page.items().at(0).name() == "Tatort");
    assert(page.items().at(1).name() == "Tagesschau");

    SearchTimerResult tail = service.list(
        timers,
        SearchTimerQuery::limited(10, 3));
    assert(tail.returnedCount() == 1);
    assert(tail.totalCount() == 4);
    assert(tail.items().at(0).name() == "Science");

    std::cout << "test_search_timer_service passed" << std::endl;
    return 0;
}