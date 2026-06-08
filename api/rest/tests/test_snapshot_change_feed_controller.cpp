#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedEntry.h"
#include "SnapshotChangeFeedJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

static void test_controller_returns_json_feed()
{
    SnapshotChangeFeed feed;
    feed.addEntry(SnapshotChangeFeedEntry(
        3,
        9,
        {"status", "events"}));

    SnapshotChangeFeedJsonSerializer serializer;
    SnapshotChangeFeedController controller(
        feed,
        serializer);

    const auto response = controller.getFeed();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"latestSequenceNumber\":3") != std::string::npos);
    assert(response.body.find("\"latestSnapshotGeneration\":9") != std::string::npos);
    assert(response.body.find("\"changedDomains\":[\"status\",\"events\"]") != std::string::npos);
}

static void test_controller_returns_empty_feed()
{
    SnapshotChangeFeed feed;
    SnapshotChangeFeedJsonSerializer serializer;
    SnapshotChangeFeedController controller(
        feed,
        serializer);

    const auto response = controller.getFeed();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body == "{\"latestSequenceNumber\":0,\"latestSnapshotGeneration\":0,\"entries\":[]}");
}

int main()
{
    test_controller_returns_json_feed();
    test_controller_returns_empty_feed();

    std::cout
        << "test_snapshot_change_feed_controller passed"
        << std::endl;

    return 0;
}
