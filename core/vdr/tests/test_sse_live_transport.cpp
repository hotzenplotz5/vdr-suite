#include "SseLiveTransport.h"
#include "DeviceLiveEventHub.h"

#include <cassert>
#include <chrono>
#include <memory>
#include <iostream>
#include <string>

static void test_transport_starts_empty()
{
    SseLiveTransport transport;

    assert(transport.empty() == true);
    assert(transport.frames().empty() == true);
    assert(transport.stream().empty() == true);
}

static void test_transport_serializes_live_update_event_as_sse_frame()
{
    SseLiveTransport transport;

    transport.publish(LiveUpdateEvent(
        7,
        42,
        {"channels", "recordings"},
        "home-vdr"));

    assert(transport.empty() == false);
    assert(transport.frames().size() == 1);

    const std::string frame = transport.frames()[0];

    assert(frame.find("event: update\n") != std::string::npos);
    assert(frame.find("id: " + transport.daemonEpoch() + ":7\n") != std::string::npos);
    assert(frame.find("data: {\"sequenceNumber\":7") != std::string::npos);
    assert(frame.find("\"snapshotGeneration\":42") != std::string::npos);
    assert(frame.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(frame.find("\"changedDomains\":[\"channels\",\"recordings\"]") != std::string::npos);
    assert(frame.size() >= 2);
    assert(frame.substr(frame.size() - 2) == "\n\n");
}

static void test_transport_serializes_searchtimer_update_as_sse_frame()
{
    SseLiveTransport transport;

    transport.publish(LiveUpdateEvent(
        9,
        44,
        {"searchtimers"},
        "home-vdr"));

    const std::string frame = transport.frames()[0];

    assert(frame.find("event: update\n") != std::string::npos);
    assert(frame.find("id: " + transport.daemonEpoch() + ":9\n") != std::string::npos);
    assert(frame.find("\"changedDomains\":[\"searchtimers\"]") != std::string::npos);
}

static void test_transport_preserves_multiple_frames_in_stream()
{
    SseLiveTransport transport;

    transport.publish(LiveUpdateEvent(
        1,
        10,
        {"status"},
        "home-vdr"));

    transport.publish(LiveUpdateEvent(
        2,
        11,
        {"timers"},
        "ferienhaus-vdr"));

    assert(transport.frames().size() == 2);

    const std::string stream = transport.stream();

    assert(stream.find("id: " + transport.daemonEpoch() + ":1\n") != std::string::npos);
    assert(stream.find("id: " + transport.daemonEpoch() + ":2\n") != std::string::npos);
    assert(stream.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(stream.find("\"backendId\":\"ferienhaus-vdr\"") != std::string::npos);
}

static void test_transport_can_be_cleared()
{
    SseLiveTransport transport;

    transport.publish(LiveUpdateEvent(
        3,
        12,
        {"events"},
        "home-vdr"));

    assert(transport.empty() == false);

    transport.clear();

    assert(transport.empty() == true);
    assert(transport.frames().empty() == true);
    assert(transport.stream().empty() == true);
}

static void test_device_subscriber_filters_backend_and_grant_then_coalesces_cursor()
{
    auto hub = std::make_shared<DeviceLiveEventHub>();
    SseLiveTransport transport;
    transport.setDeviceLiveEventHub(hub);
    auto timer = hub->subscribe("home-vdr", {"timers"});
    auto recording = hub->subscribe("home-vdr", {"recordings"});
    auto other = hub->subscribe("other-vdr", {"timers"});
    assert(timer && recording && other);
    std::string frame;
    assert(timer->next(frame, std::chrono::milliseconds(1)));
    assert(frame.find("event: hello") != std::string::npos);
    assert(frame.find("\"authorizedDomains\":[\"timers\"]") != std::string::npos);
    assert(recording->next(frame, std::chrono::milliseconds(1)));
    assert(frame.find("\"authorizedDomains\":[\"recordings\"]") != std::string::npos);
    assert(other->next(frame, std::chrono::milliseconds(1)));

    // Wrong backend, EPG and overlay do not advance subscriber cursors.
    transport.publish(LiveUpdateEvent(1, 1, {"timers"}, "other-vdr"));
    transport.publish(LiveUpdateEvent(2, 2, {"events"}, "home-vdr"));
    transport.publish(LiveUpdateEvent(3, 3, {"timers", "recordingMarks"},
                                      "home-vdr"));
    assert(timer->next(frame, std::chrono::milliseconds(1)));
    assert(frame.find("event: update") != std::string::npos);
    assert(frame.find("\"subscriptionSequence\":1") != std::string::npos);
    assert(frame.find("\"changedDomains\":[\"timers\"]") != std::string::npos);
    assert(recording->next(frame, std::chrono::milliseconds(1)));
    assert(frame.find("\"changedDomains\":[\"recordings\"]") != std::string::npos);
    assert(other->next(frame, std::chrono::milliseconds(1)));
    assert(frame.find("\"backendId\":\"other-vdr\"") != std::string::npos);

    // No replaying EPG/other-backend frames after consuming a subscription.
    assert(timer->next(frame, std::chrono::milliseconds(1)) && frame.empty());
    assert(recording->next(frame, std::chrono::milliseconds(1)) && frame.empty());
    transport.publish(LiveUpdateEvent(4, 4, {"recordings"}, "home-vdr"));
    assert(timer->next(frame, std::chrono::milliseconds(1)) && frame.empty());
    assert(recording->next(frame, std::chrono::milliseconds(1)));
    assert(frame.find("\"subscriptionSequence\":2") != std::string::npos);

    hub->disconnectAll();
    assert(!timer->next(frame, std::chrono::milliseconds(1)));
    assert(!recording->next(frame, std::chrono::milliseconds(1)));
    assert(!other->next(frame, std::chrono::milliseconds(1)));
}

static void test_device_subscriber_overload_requires_resync_not_silent_drop()
{
    DeviceLiveEventHub hub;
    auto subscriber = hub.subscribe("home", {"timers"});
    std::string frame;
    assert(subscriber->next(frame, std::chrono::milliseconds(1)));
    for (int i = 1; i <= 100; ++i)
        hub.publish(LiveUpdateEvent(i, i, {"timers"}, "home"));
    assert(subscriber->next(frame, std::chrono::milliseconds(1)));
    assert(frame.find("event: resync") != std::string::npos);
    assert(!subscriber->next(frame, std::chrono::milliseconds(1)));
}


static void test_cursor_replay_gap_and_restart()
{
    SseLiveTransport transport;
    assert(transport.replay("").find("event: resync\n") != std::string::npos);
    for (int i = 1; i <= 3; ++i)
        transport.publish(LiveUpdateEvent(i, i, {"recordings"},
                                          i == 2 ? "secondary" : "home"));
    const std::string cursor = transport.daemonEpoch() + ":1";
    const std::string delta = transport.replay(cursor);
    assert(delta.find("\"backendId\":\"secondary\"") != std::string::npos);
    assert(delta.find("id: " + transport.daemonEpoch() + ":1\n") == std::string::npos);
    assert(delta.find("id: " + transport.daemonEpoch() + ":2\n") != std::string::npos);
    assert(delta.find("id: " + transport.daemonEpoch() + ":3\n") != std::string::npos);
    assert(transport.replay(transport.daemonEpoch() + ":3").find("event: update") ==
           std::string::npos);
    assert(transport.replay(transport.daemonEpoch() + ":99").find("invalid_cursor") !=
           std::string::npos);
    assert(transport.replay(transport.daemonEpoch() + ":invalid").find("invalid_cursor") !=
           std::string::npos);
    assert(transport.replay(transport.daemonEpoch() + ":01").find("invalid_cursor") !=
           std::string::npos);
    SseLiveTransport restarted;
    assert(restarted.replay(cursor).find("epoch_changed") != std::string::npos);
}

static void test_bounded_history_and_missing_sequence()
{
    SseLiveTransport transport;
    for (int i = 1; i <= 2048; ++i)
        transport.publish(LiveUpdateEvent(i, i, {"recordings"}, "home"));
    assert(transport.frames().size() <= SseLiveTransport::maxRetainedFrames);
    assert(transport.stream().size() <= SseLiveTransport::maxRetainedBytes);
    assert(transport.latestSequenceNumber() == 2048);
    assert(transport.oldestSequenceNumber() > 1);
    assert(transport.replay(transport.daemonEpoch() + ":1").find("event: resync") !=
           std::string::npos);
    const std::string newest = transport.replay(transport.daemonEpoch() + ":2047");
    assert(newest.find(":2048\n") != std::string::npos);
    assert(newest.find(":2047\n") == std::string::npos);
    transport.publish(LiveUpdateEvent(2050, 2050, {"timers"}, "home"));
    assert(transport.replay(transport.daemonEpoch() + ":2048").find("event: resync") !=
           std::string::npos);
    transport.clear();
    assert(transport.latestSequenceNumber() == 2050);
    assert(transport.replay(transport.daemonEpoch() + ":2048").find("event: resync") !=
           std::string::npos);
    transport.publish(LiveUpdateEvent(2051, 2051,
        {std::string(SseLiveTransport::maxRetainedBytes + 1, 'x')}, "home"));
    assert(transport.frames().empty());
    assert(transport.latestSequenceNumber() == 2051);
}

int main()
{
    test_device_subscriber_filters_backend_and_grant_then_coalesces_cursor();
    test_device_subscriber_overload_requires_resync_not_silent_drop();
    test_cursor_replay_gap_and_restart();
    test_bounded_history_and_missing_sequence();
    test_transport_starts_empty();
    test_transport_serializes_live_update_event_as_sse_frame();
    test_transport_serializes_searchtimer_update_as_sse_frame();
    test_transport_preserves_multiple_frames_in_stream();
    test_transport_can_be_cleared();

    std::cout
        << "test_sse_live_transport passed"
        << std::endl;

    return 0;
}
