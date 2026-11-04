#include "LiveTransportController.h"
#include "LiveUpdateEvent.h"
#include "SseLiveTransport.h"
#include "TestLiveTransport.h"

#include <cassert>
#include <iostream>
#include <string>

static void test_controller_returns_sse_stream_from_interface()
{
    SseLiveTransport transport;

    transport.publish(LiveUpdateEvent(
        7,
        42,
        {"channels"},
        "home-vdr"));

    LiveTransportController controller(transport);

    const auto response = controller.getStream(transport.daemonEpoch() + ":6");

    assert(response.statusCode == 200);
    assert(response.contentType == "text/event-stream");
    assert(response.body.find("event: update\n") != std::string::npos);
    assert(response.body.find("id: " + transport.daemonEpoch() + ":7\n") != std::string::npos);
    assert(response.body.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(response.body.find("\"changedDomains\":[\"channels\"]") != std::string::npos);
}

static void test_controller_can_use_test_transport_through_interface()
{
    TestLiveTransport transport;

    transport.publish(LiveUpdateEvent(
        8,
        43,
        {"timers"},
        "ferienhaus-vdr"));

    LiveTransportController controller(transport);

    const auto response = controller.getStream();

    assert(response.statusCode == 200);
    assert(response.contentType == "text/event-stream");
    assert(response.body.find("8:ferienhaus-vdr") != std::string::npos);
}

static void test_controller_returns_empty_stream()
{
    SseLiveTransport transport;
    LiveTransportController controller(transport);

    const auto response = controller.getStream();

    assert(response.statusCode == 200);
    assert(response.contentType == "text/event-stream");
    assert(response.body.find("event: resync\n") != std::string::npos);
    assert(response.headers.at("Cache-Control") == "no-store");
}

int main()
{
    test_controller_returns_sse_stream_from_interface();
    test_controller_can_use_test_transport_through_interface();
    test_controller_returns_empty_stream();

    std::cout
        << "test_live_transport_controller passed"
        << std::endl;

    return 0;
}
