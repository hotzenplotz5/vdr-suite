#include "DeviceLiveEventHub.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>

static std::string consume(DeviceLiveEventSubscription& subscriber)
{
    std::string frame;
    assert(subscriber.next(frame, std::chrono::milliseconds(2)));
    return frame;
}

static void test_grant_scoped_backend_filter_and_cursor()
{
    DeviceLiveEventHub hub;
    auto timers = hub.subscribe("vdr-a", {"timers"});
    auto recordings = hub.subscribe("vdr-a", {"recordings"});
    auto foreign = hub.subscribe("vdr-b", {"timers"});
    assert(timers && recordings && foreign);
    assert(consume(*timers).find("event: hello") != std::string::npos);
    assert(consume(*recordings).find("\"recordings\"") != std::string::npos);
    consume(*foreign);

    hub.publish(LiveUpdateEvent(77, 5, {"events"}, "vdr-a"));
    hub.publish(LiveUpdateEvent(78, 5, {"timers"}, "vdr-b"));
    hub.publish(LiveUpdateEvent(79, 5, {"timers", "recordingMarks"}, "vdr-a"));
    const auto timer = consume(*timers);
    const auto recording = consume(*recordings);
    const auto second = consume(*foreign);
    assert(timer.find("\"changedDomains\":[\"timers\"]") != std::string::npos);
    assert(timer.find("\"subscriptionSequence\":1") != std::string::npos);
    assert(recording.find("\"changedDomains\":[\"recordings\"]") !=
           std::string::npos);
    assert(second.find("\"backendId\":\"vdr-b\"") != std::string::npos);
    std::string idle;
    assert(timers->next(idle, std::chrono::milliseconds(1)));
    assert(idle.empty());
    hub.publish(LiveUpdateEvent(80, 8, {"recordings"}, "vdr-a"));
    assert(recordings->next(idle, std::chrono::milliseconds(1)));
    assert(idle.find("\"subscriptionSequence\":2") != std::string::npos);
    hub.disconnectAll();
    assert(!timers->next(idle, std::chrono::milliseconds(1)));
    assert(!recordings->next(idle, std::chrono::milliseconds(1)));
    assert(!foreign->next(idle, std::chrono::milliseconds(1)));
}

static void test_overload_reports_resync_and_bounds_waiting_payload()
{
    DeviceLiveEventHub hub;
    auto subscriber = hub.subscribe("home", {"timers"});
    assert(subscriber);
    consume(*subscriber);
    for (int i = 1; i <= 130; ++i)
        hub.publish(LiveUpdateEvent(i, 6, {"timers"}, "home"));
    const auto frame = consume(*subscriber);
    assert(frame.find("event: resync") != std::string::npos);
    std::string after;
    assert(!subscriber->next(after, std::chrono::milliseconds(1)));
}

int main()
{
    test_grant_scoped_backend_filter_and_cursor();
    test_overload_reports_resync_and_bounds_waiting_payload();
    std::cout << "test_device_live_event_hub passed\n";
    return 0;
}
