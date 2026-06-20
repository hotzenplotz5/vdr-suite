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

    assert(activeTimer.recordingOptions().directory().empty());
    assert(activeTimer.recordingOptions().priority() == 0);
    assert(activeTimer.recordingOptions().lifetime() == 0);
    assert(activeTimer.scheduleOptions().marginStartMinutes() == 0);
    assert(activeTimer.scheduleOptions().marginStopMinutes() == 0);
    assert(!activeTimer.scheduleOptions().useVps());

    activeTimer.recordingOptions().setDirectory("Doku");
    activeTimer.recordingOptions().setPriority(50);
    activeTimer.recordingOptions().setLifetime(99);
    activeTimer.scheduleOptions().setMarginStartMinutes(5);
    activeTimer.scheduleOptions().setMarginStopMinutes(10);
    activeTimer.scheduleOptions().setUseVps(true);

    assert(activeTimer.recordingOptions().directory() == "Doku");
    assert(activeTimer.recordingOptions().priority() == 50);
    assert(activeTimer.recordingOptions().lifetime() == 99);
    assert(activeTimer.scheduleOptions().marginStartMinutes() == 5);
    assert(activeTimer.scheduleOptions().marginStopMinutes() == 10);
    assert(activeTimer.scheduleOptions().useVps());

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