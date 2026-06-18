#include "VdrTimerOperationRequest.h"

#include <cassert>

static void test_defaults_are_vdr_safe()
{
    VdrTimerOperationRequest request;

    assert(request.timerId.empty() == true);
    assert(request.backendId.empty() == true);
    assert(request.channelId.empty() == true);
    assert(request.title.empty() == true);
    assert(request.directory.empty() == true);
    assert(request.day.empty() == true);
    assert(request.weekdays == "-------");
    assert(request.start == 0);
    assert(request.stop == 0);
    assert(request.priority == 50);
    assert(request.lifetime == 99);
    assert(request.active == true);
    assert(request.vps == false);
    assert(request.aux.empty() == true);

    assert(request.hasTimerId() == false);
    assert(request.hasBackendId() == false);
    assert(request.hasDirectory() == false);
    assert(request.hasDay() == false);
    assert(request.isRepeating() == false);
    assert(request.hasTimeWindow() == false);
    assert(request.isActive() == true);
    assert(request.usesVps() == false);
}

static void test_single_event_timer_request()
{
    VdrTimerOperationRequest request;

    request.backendId = "living-room";
    request.channelId = "C-61441-10006-50021";
    request.title = "Tagesschau";
    request.day = "2026-06-18";
    request.start = 2015;
    request.stop = 2030;
    request.priority = 50;
    request.lifetime = 99;

    assert(request.hasTimerId() == false);
    assert(request.hasBackendId() == true);
    assert(request.hasDay() == true);
    assert(request.isRepeating() == false);
    assert(request.hasTimeWindow() == true);
    assert(request.isActive() == true);
    assert(request.usesVps() == false);
}

static void test_update_request_has_timer_id()
{
    VdrTimerOperationRequest request;

    request.timerId = "42";
    request.backendId = "living-room";
    request.channelId = "C-61441-10006-50021";
    request.title = "Tagesschau";
    request.day = "2026-06-18";
    request.start = 2015;
    request.stop = 2030;

    assert(request.hasTimerId() == true);
    assert(request.hasBackendId() == true);
    assert(request.hasTimeWindow() == true);
}

static void test_repeating_timer_request()
{
    VdrTimerOperationRequest request;

    request.channelId = "C-61441-10006-50021";
    request.title = "Daily News";
    request.weekdays = "MTWTF--";
    request.start = 2015;
    request.stop = 2030;

    assert(request.isRepeating() == true);
    assert(request.hasDay() == false);
    assert(request.hasTimeWindow() == true);
}

static void test_directory_is_separate_from_title()
{
    VdrTimerOperationRequest request;

    request.directory = "News";
    request.title = "Tagesschau";

    assert(request.hasDirectory() == true);
    assert(request.directory == "News");
    assert(request.title == "Tagesschau");
}

static void test_vps_is_explicit_flag()
{
    VdrTimerOperationRequest request;

    request.vps = true;

    assert(request.usesVps() == true);
}

int main()
{
    test_defaults_are_vdr_safe();
    test_single_event_timer_request();
    test_update_request_has_timer_id();
    test_repeating_timer_request();
    test_directory_is_separate_from_title();
    test_vps_is_explicit_flag();

    return 0;
}
