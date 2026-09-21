#include "TestLiveTransport.h"

#include <cassert>
#include <iostream>

static void test_transport_starts_empty()
{
    TestLiveTransport transport;

    assert(transport.empty() == true);
    assert(transport.events().empty() == true);
}

static void test_transport_publishes_live_update_event()
{
    TestLiveTransport transport;

    LiveUpdateEvent event(
        7,
        42,
        {"channels"},
        "home-vdr");

    transport.publish(event);

    assert(transport.empty() == false);
    assert(transport.events().size() == 1);
    assert(transport.events()[0].sequenceNumber() == 7);
    assert(transport.events()[0].snapshotGeneration() == 42);
    assert(transport.events()[0].backendId() == "home-vdr");
    assert(transport.events()[0].changedDomains().size() == 1);
    assert(transport.events()[0].changedDomains()[0] == "channels");
}

static void test_transport_preserves_multiple_events()
{
    TestLiveTransport transport;

    transport.publish(LiveUpdateEvent(
        1,
        10,
        {"status"},
        "home-vdr"));

    transport.publish(LiveUpdateEvent(
        2,
        11,
        {"recordings"},
        "ferienhaus-vdr"));

    assert(transport.events().size() == 2);

    assert(transport.events()[0].sequenceNumber() == 1);
    assert(transport.events()[0].backendId() == "home-vdr");
    assert(transport.events()[0].changedDomains()[0] == "status");

    assert(transport.events()[1].sequenceNumber() == 2);
    assert(transport.events()[1].backendId() == "ferienhaus-vdr");
    assert(transport.events()[1].changedDomains()[0] == "recordings");
}

static void test_transport_can_be_cleared()
{
    TestLiveTransport transport;

    transport.publish(LiveUpdateEvent(
        3,
        12,
        {"timers"},
        "home-vdr"));

    assert(transport.empty() == false);

    transport.clear();

    assert(transport.empty() == true);
    assert(transport.events().empty() == true);
}

int main()
{
    test_transport_starts_empty();
    test_transport_publishes_live_update_event();
    test_transport_preserves_multiple_events();
    test_transport_can_be_cleared();

    std::cout
        << "test_test_live_transport passed"
        << std::endl;

    return 0;
}
