#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedEntry.h"
#include "SnapshotChangeFeedJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

static void test_serializer_serializes_empty_feed()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedJsonSerializer serializer;

    const auto json = serializer.serializeFeed(feed);

    assert(json == "{\"latestSequenceNumber\":0,\"latestSnapshotGeneration\":0,\"entries\":[]}");
}

static void test_serializer_serializes_single_entry()
{
    SnapshotChangeFeedEntry entry(
        7,
        3,
        {"status", "recordings"});

    SnapshotChangeFeedJsonSerializer serializer;

    const auto json = serializer.serializeEntry(entry);

    assert(json.find("\"sequenceNumber\":7") != std::string::npos);
    assert(json.find("\"snapshotGeneration\":3") != std::string::npos);
    assert(json.find("\"changedDomains\":[\"status\",\"recordings\"]") != std::string::npos);
}

static void test_serializer_serializes_feed_entries()
{
    SnapshotChangeFeed feed;
    feed.addEntry(SnapshotChangeFeedEntry(1, 10, {"channels"}));
    feed.addEntry(SnapshotChangeFeedEntry(2, 11, {"timers", "events"}));

    SnapshotChangeFeedJsonSerializer serializer;

    const auto json = serializer.serializeFeed(feed);

    assert(json.find("\"latestSequenceNumber\":2") != std::string::npos);
    assert(json.find("\"latestSnapshotGeneration\":11") != std::string::npos);
    assert(json.find("\"entries\":[") != std::string::npos);
    assert(json.find("\"changedDomains\":[\"channels\"]") != std::string::npos);
    assert(json.find("\"changedDomains\":[\"timers\",\"events\"]") != std::string::npos);
}

int main()
{
    test_serializer_serializes_empty_feed();
    test_serializer_serializes_single_entry();
    test_serializer_serializes_feed_entries();

    std::cout
        << "test_snapshot_change_feed_json_serializer passed"
        << std::endl;

    return 0;
}
