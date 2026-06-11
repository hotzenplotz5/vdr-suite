#include "LiveUpdateEvent.h"
#include "SnapshotChangeFeedEntry.h"

#include <cassert>
#include <iostream>

static void test_event_stores_live_update_fields()
{
    LiveUpdateEvent event(
        7,
        42,
        {"channels", "recordings"},
        "home-vdr");

    assert(event.sequenceNumber() == 7);
    assert(event.snapshotGeneration() == 42);
    assert(event.backendId() == "home-vdr");
    assert(event.changedDomains().size() == 2);
    assert(event.changedDomains()[0] == "channels");
    assert(event.changedDomains()[1] == "recordings");
    assert(event.hasChanges() == true);
}

static void test_event_uses_default_backend_id()
{
    LiveUpdateEvent event(
        1,
        2,
        {"status"});

    assert(event.backendId() == "default");
}

static void test_event_can_be_created_from_snapshot_change_feed_entry()
{
    SnapshotChangeFeedEntry entry(
        8,
        43,
        {"timers"},
        "ferienhaus-vdr");

    LiveUpdateEvent event(entry);

    assert(event.sequenceNumber() == 8);
    assert(event.snapshotGeneration() == 43);
    assert(event.backendId() == "ferienhaus-vdr");
    assert(event.changedDomains().size() == 1);
    assert(event.changedDomains()[0] == "timers");
}

static void test_empty_event_has_no_changes()
{
    LiveUpdateEvent event(
        9,
        44,
        {},
        "home-vdr");

    assert(event.hasChanges() == false);
}

int main()
{
    test_event_stores_live_update_fields();
    test_event_uses_default_backend_id();
    test_event_can_be_created_from_snapshot_change_feed_entry();
    test_empty_event_has_no_changes();

    std::cout
        << "test_live_update_event passed"
        << std::endl;

    return 0;
}
