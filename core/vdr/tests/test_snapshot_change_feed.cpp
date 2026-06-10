#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedService.h"
#include "VdrChangeEvent.h"

#include <cassert>
#include <iostream>

static void test_entry_stores_sequence_generation_backend_and_domains()
{
    SnapshotChangeFeedEntry entry(
        7,
        3,
        {"status", "recordings"});

    assert(entry.sequenceNumber() == 7);
    assert(entry.snapshotGeneration() == 3);
    assert(entry.backendId() == "default");
    assert(entry.hasChanges() == true);
    assert(entry.changedDomains().size() == 2);
    assert(entry.changedDomains()[0] == "status");
    assert(entry.changedDomains()[1] == "recordings");
}

static void test_entry_stores_explicit_backend_id()
{
    SnapshotChangeFeedEntry entry(
        8,
        4,
        {"timers"},
        "parents-vdr");

    assert(entry.sequenceNumber() == 8);
    assert(entry.snapshotGeneration() == 4);
    assert(entry.backendId() == "parents-vdr");
    assert(entry.changedDomains().size() == 1);
    assert(entry.changedDomains()[0] == "timers");
}

static void test_feed_tracks_latest_entry()
{
    SnapshotChangeFeed feed;

    assert(feed.empty() == true);
    assert(feed.latestSequenceNumber() == 0);
    assert(feed.latestSnapshotGeneration() == 0);

    feed.addEntry(SnapshotChangeFeedEntry(1, 10, {"status"}));
    feed.addEntry(SnapshotChangeFeedEntry(2, 11, {"timers"}));

    assert(feed.empty() == false);
    assert(feed.entries().size() == 2);
    assert(feed.latestSequenceNumber() == 2);
    assert(feed.latestSnapshotGeneration() == 11);
    assert(feed.entries()[0].backendId() == "default");
    assert(feed.entries()[1].backendId() == "default");
}

static void test_service_creates_feed_from_change_events()
{
    SnapshotChangeFeedService service;

    const auto feed = service.createFeed(
        5,
        12,
        {
            VdrChangeEvent(VdrChangeType::ChannelsChanged),
            VdrChangeEvent(VdrChangeType::EventsChanged)
        });

    assert(feed.empty() == false);
    assert(feed.entries().size() == 1);
    assert(feed.latestSequenceNumber() == 5);
    assert(feed.latestSnapshotGeneration() == 12);
    assert(feed.entries()[0].backendId() == "default");
    assert(feed.entries()[0].changedDomains().size() == 2);
    assert(feed.entries()[0].changedDomains()[0] == "channels");
    assert(feed.entries()[0].changedDomains()[1] == "events");
}

static void test_service_does_not_add_empty_feed_entry()
{
    SnapshotChangeFeedService service;

    const auto feed = service.createFeed(1, 1, {});

    assert(feed.empty() == true);
    assert(feed.entries().empty() == true);
}


static void test_change_feed_service_preserves_backend_id()
{
    SnapshotChangeFeedService service;

    const auto entry = service.createEntry(
        1,
        42,
        { VdrChangeEvent(VdrChangeType::ChannelsChanged) },
        "parents-vdr");

    assert(entry.hasChanges());
    assert(entry.backendId() == "parents-vdr");
    assert(entry.snapshotGeneration() == 42);
    assert(entry.changedDomains().size() == 1);
    assert(entry.changedDomains()[0] == "channels");
}


int main()
{
    test_entry_stores_sequence_generation_backend_and_domains();
    test_entry_stores_explicit_backend_id();
    test_feed_tracks_latest_entry();
    test_service_creates_feed_from_change_events();
    test_service_does_not_add_empty_feed_entry();

    std::cout
        << "test_snapshot_change_feed passed"
        << std::endl;

    return 0;
}
