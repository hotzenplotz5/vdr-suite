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


static void test_append_changes_adds_next_sequence_number()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    service.appendChanges(
        feed,
        10,
        { VdrChangeEvent(VdrChangeType::StatusChanged) },
        "default");

    assert(feed.empty() == false);
    assert(feed.entries().size() == 1);
    assert(feed.latestSequenceNumber() == 1);
    assert(feed.latestSnapshotGeneration() == 10);
    assert(feed.entries()[0].backendId() == "default");
    assert(feed.entries()[0].changedDomains().size() == 1);
    assert(feed.entries()[0].changedDomains()[0] == "status");
}

static void test_append_changes_increments_sequence_number()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    service.appendChanges(
        feed,
        10,
        { VdrChangeEvent(VdrChangeType::StatusChanged) },
        "default");

    service.appendChanges(
        feed,
        11,
        { VdrChangeEvent(VdrChangeType::RecordingsChanged) },
        "default");

    assert(feed.entries().size() == 2);
    assert(feed.entries()[0].sequenceNumber() == 1);
    assert(feed.entries()[1].sequenceNumber() == 2);
    assert(feed.latestSequenceNumber() == 2);
    assert(feed.latestSnapshotGeneration() == 11);
    assert(feed.entries()[1].changedDomains().size() == 1);
    assert(feed.entries()[1].changedDomains()[0] == "recordings");
}

static void test_append_changes_does_not_add_empty_entry()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    service.appendChanges(
        feed,
        10,
        {},
        "default");

    assert(feed.empty() == true);
    assert(feed.entries().empty() == true);
    assert(feed.latestSequenceNumber() == 0);
    assert(feed.latestSnapshotGeneration() == 0);
}

static void test_append_changes_preserves_backend_id()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    service.appendChanges(
        feed,
        15,
        { VdrChangeEvent(VdrChangeType::TimersChanged) },
        "parents-vdr");

    assert(feed.entries().size() == 1);
    assert(feed.entries()[0].backendId() == "parents-vdr");
    assert(feed.entries()[0].snapshotGeneration() == 15);
    assert(feed.entries()[0].changedDomains().size() == 1);
    assert(feed.entries()[0].changedDomains()[0] == "timers");
}

static void test_append_changes_preserves_multiple_changed_domains()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    service.appendChanges(
        feed,
        20,
        {
            VdrChangeEvent(VdrChangeType::ChannelsChanged),
            VdrChangeEvent(VdrChangeType::EventsChanged),
            VdrChangeEvent(VdrChangeType::RecordingsChanged)
        },
        "default");

    assert(feed.entries().size() == 1);
    assert(feed.entries()[0].changedDomains().size() == 3);
    assert(feed.entries()[0].changedDomains()[0] == "channels");
    assert(feed.entries()[0].changedDomains()[1] == "events");
    assert(feed.entries()[0].changedDomains()[2] == "recordings");
}


static void test_append_changes_preserves_multiple_backend_entries()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedService service;

    service.appendChanges(
        feed,
        21,
        { VdrChangeEvent(VdrChangeType::ChannelsChanged) },
        "home-vdr");

    service.appendChanges(
        feed,
        22,
        { VdrChangeEvent(VdrChangeType::RecordingsChanged) },
        "ferienhaus-vdr");

    assert(feed.entries().size() == 2);

    assert(feed.entries()[0].sequenceNumber() == 1);
    assert(feed.entries()[0].snapshotGeneration() == 21);
    assert(feed.entries()[0].backendId() == "home-vdr");
    assert(feed.entries()[0].changedDomains().size() == 1);
    assert(feed.entries()[0].changedDomains()[0] == "channels");

    assert(feed.entries()[1].sequenceNumber() == 2);
    assert(feed.entries()[1].snapshotGeneration() == 22);
    assert(feed.entries()[1].backendId() == "ferienhaus-vdr");
    assert(feed.entries()[1].changedDomains().size() == 1);
    assert(feed.entries()[1].changedDomains()[0] == "recordings");

    assert(feed.latestSequenceNumber() == 2);
    assert(feed.latestSnapshotGeneration() == 22);
}

int main()
{
    test_entry_stores_sequence_generation_backend_and_domains();
    test_entry_stores_explicit_backend_id();
    test_feed_tracks_latest_entry();
    test_service_creates_feed_from_change_events();
    test_service_does_not_add_empty_feed_entry();
    test_change_feed_service_preserves_backend_id();
    test_append_changes_adds_next_sequence_number();
    test_append_changes_increments_sequence_number();
    test_append_changes_does_not_add_empty_entry();
    test_append_changes_preserves_backend_id();
    test_append_changes_preserves_multiple_changed_domains();
    test_append_changes_preserves_multiple_backend_entries();

    std::cout
        << "test_snapshot_change_feed passed"
        << std::endl;

    return 0;
}
