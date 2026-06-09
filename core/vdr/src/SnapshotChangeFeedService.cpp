#include "SnapshotChangeFeedService.h"

static std::string domainNameForChangeType(VdrChangeType type)
{
    switch (type) {
    case VdrChangeType::StatusChanged:
        return "status";
    case VdrChangeType::ChannelsChanged:
        return "channels";
    case VdrChangeType::RecordingsChanged:
        return "recordings";
    case VdrChangeType::TimersChanged:
        return "timers";
    case VdrChangeType::EventsChanged:
        return "events";
    }

    return "";
}

SnapshotChangeFeedEntry SnapshotChangeFeedService::createEntry(
    int sequenceNumber,
    int snapshotGeneration,
    const std::vector<VdrChangeEvent>& changeEvents) const
{
    std::vector<std::string> changedDomains;

    for (const auto& event : changeEvents) {
        changedDomains.push_back(domainNameForChangeType(event.type()));
    }

    return SnapshotChangeFeedEntry(
        sequenceNumber,
        snapshotGeneration,
        changedDomains);
}

SnapshotChangeFeed SnapshotChangeFeedService::createFeed(
    int sequenceNumber,
    int snapshotGeneration,
    const std::vector<VdrChangeEvent>& changeEvents) const
{
    SnapshotChangeFeed feed;

    const auto entry = createEntry(
        sequenceNumber,
        snapshotGeneration,
        changeEvents);

    if (entry.hasChanges()) {
        feed.addEntry(entry);
    }

    return feed;
}

void SnapshotChangeFeedService::appendChanges(
    SnapshotChangeFeed& feed,
    int snapshotGeneration,
    const std::vector<VdrChangeEvent>& changeEvents) const
{
    const int nextSequenceNumber = feed.latestSequenceNumber() + 1;
    const auto entry = createEntry(nextSequenceNumber, snapshotGeneration, changeEvents);

    if (entry.hasChanges()) {
        feed.addEntry(entry);
    }
}
