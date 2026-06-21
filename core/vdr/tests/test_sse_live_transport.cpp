#include "SseLiveTransport.h"

#include <cassert>
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
    assert(frame.find("id: 7\n") != std::string::npos);
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
    assert(frame.find("id: 9\n") != std::string::npos);
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

    assert(stream.find("id: 1\n") != std::string::npos);
    assert(stream.find("id: 2\n") != std::string::npos);
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

int main()
{
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
