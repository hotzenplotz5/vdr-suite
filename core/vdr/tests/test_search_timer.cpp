#include "SearchTimer.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerId invalidId = SearchTimerId::fromBackendNativeId("", "");
    assert(!invalidId.isValid());

    SearchTimerId id = SearchTimerId::fromBackendNativeId("livingroom", "42");
    assert(id.isValid());
    assert(id.backendId() == "livingroom");
    assert(id.nativeId() == "42");

    SearchTimerId sameId = SearchTimerId::fromBackendNativeId("livingroom", "42");
    SearchTimerId differentBackend = SearchTimerId::fromBackendNativeId("bedroom", "42");
    SearchTimerId differentNativeId = SearchTimerId::fromBackendNativeId("livingroom", "43");

    assert(id == sameId);
    assert(id != differentBackend);
    assert(id != differentNativeId);

    SearchTimer activeTimer = SearchTimer::create(
        id,
        "Terra X",
        "Terra X",
        SearchTimerState::Active);

    assert(activeTimer.id() == id);
    assert(activeTimer.backendId() == "livingroom");
    assert(activeTimer.backendNativeId() == "42");
    assert(activeTimer.name() == "Terra X");
    assert(activeTimer.query() == "Terra X");
    assert(activeTimer.state() == SearchTimerState::Active);
    assert(activeTimer.isActive());
    assert(!activeTimer.isInactive());

    SearchTimer inactiveTimer = SearchTimer::create(
        SearchTimerId::fromBackendNativeId("livingroom", "43"),
        "Tatort",
        "Tatort",
        SearchTimerState::Inactive);

    assert(inactiveTimer.state() == SearchTimerState::Inactive);
    assert(!inactiveTimer.isActive());
    assert(inactiveTimer.isInactive());

    std::cout << "test_search_timer passed" << std::endl;
    return 0;
}