#include "LiveTransportService.h"
#include "SnapshotChangeFeedEntry.h"
#include "TestLiveTransport.h"

#include <cassert>
#include <iostream>

static void test_service_publishes_live_update_event()
{
    TestLiveTransport transport;
    LiveTransportService service(transport);

    service.publish(LiveUpdateEvent(
        7,
        42,
        {"channels"},
        "home-vdr"));

    assert(transport.events().size() == 1);
    assert(transport.events()[0].sequenceNumber() == 7);
    assert(transport.events()[0].snapshotGeneration() == 42);
    assert(transport.events()[0].backendId() == "home-vdr");
    assert(transport.events()[0].changedDomains()[0] == "channels");
}

static void test_service_publishes_snapshot_change_feed_entry()
{
    TestLiveTransport transport;
    LiveTransportService service(transport);

    SnapshotChangeFeedEntry entry(
        8,
        43,
        {"recordings"},
        "ferienhaus-vdr");

    service.publishChangeFeedEntry(entry);

    assert(transport.events().size() == 1);
    assert(transport.events()[0].sequenceNumber() == 8);
    assert(transport.events()[0].snapshotGeneration() == 43);
    assert(transport.events()[0].backendId() == "ferienhaus-vdr");
    assert(transport.events()[0].changedDomains()[0] == "recordings");
}

static void test_service_skips_empty_live_update_event()
{
    TestLiveTransport transport;
    LiveTransportService service(transport);

    service.publish(LiveUpdateEvent(
        9,
        44,
        {},
        "home-vdr"));

    assert(transport.empty() == true);
}

static void test_service_skips_empty_snapshot_change_feed_entry()
{
    TestLiveTransport transport;
    LiveTransportService service(transport);

    SnapshotChangeFeedEntry entry(
        10,
        45,
        {},
        "home-vdr");

    service.publishChangeFeedEntry(entry);

    assert(transport.empty() == true);
}

int main()
{
    test_service_publishes_live_update_event();
    test_service_publishes_snapshot_change_feed_entry();
    test_service_skips_empty_live_update_event();
    test_service_skips_empty_snapshot_change_feed_entry();

    std::cout
        << "test_live_transport_service passed"
        << std::endl;

    return 0;
}
