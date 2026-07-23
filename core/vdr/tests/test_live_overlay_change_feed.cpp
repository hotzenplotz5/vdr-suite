#include "SnapshotChangeFeedService.h"

#include <algorithm>
#include <cassert>

int main()
{
    SnapshotChangeFeedService service;
    SnapshotChangeFeed feed;

    service.appendChanges(
        feed,
        10,
        {VdrChangeEvent(VdrChangeType::LiveOverlayChanged)},
        "backend-a");

    assert(feed.entries().size() == 1);
    assert(feed.entries().front().sequenceNumber() == 1);
    assert(feed.entries().front().backendId() == "backend-a");
    assert(feed.entries().front().changedDomains().size() == 1);
    assert(feed.entries().front().changedDomains().front() == "liveOverlay");

    service.appendChanges(
        feed,
        11,
        {VdrChangeEvent(VdrChangeType::EventsChanged)},
        "backend-a");

    assert(feed.entries().back().sequenceNumber() == 2);
    const auto& domains = feed.entries().back().changedDomains();
    assert(std::find(domains.begin(), domains.end(), "events") != domains.end());
    assert(std::find(domains.begin(), domains.end(), "liveOverlay") != domains.end());
    return 0;
}
