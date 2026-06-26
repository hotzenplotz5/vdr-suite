#include "SnapshotChangeFeedService.h"

#include "SearchTimerPreviewEpgCacheChangeInvalidator.h"

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
    case VdrChangeType::SearchTimersChanged:
        return "searchtimers";
    case VdrChangeType::EventsChanged:
        return "events";
    }

    return "";
}

SnapshotChangeFeedEntry SnapshotChangeFeedService::createEntry(
    int sequenceNumber,
    int snapshotGeneration,
    const std::vector<VdrChangeEvent>& changeEvents,
    const std::string& backendId) const
{
    std::vector<std::string> changedDomains;

    for (const auto& event : changeEvents) {
        changedDomains.push_back(domainNameForChangeType(event.type()));
    }

    return SnapshotChangeFeedEntry(
        sequenceNumber,
        snapshotGeneration,
        changedDomains,
        backendId);
}

SnapshotChangeFeed SnapshotChangeFeedService::createFeed(
    int sequenceNumber,
    int snapshotGeneration,
    const std::vector<VdrChangeEvent>& changeEvents,
    const std::string& backendId) const
{
    SnapshotChangeFeed feed;

    const auto entry = createEntry(
        sequenceNumber,
        snapshotGeneration,
        changeEvents,
        backendId);

    if (entry.hasChanges()) {
        feed.addEntry(entry);
    }

    return feed;
}

void SnapshotChangeFeedService::appendChanges(
    SnapshotChangeFeed& feed,
    int snapshotGeneration,
    const std::vector<VdrChangeEvent>& changeEvents,
    const std::string& backendId) const
{
    SearchTimerPreviewEpgCacheChangeInvalidator::invalidateRegisteredRuntimeCacheForChangeEvents(
        backendId,
        changeEvents);

    const int nextSequenceNumber = feed.latestSequenceNumber() + 1;
    const auto entry = createEntry(nextSequenceNumber, snapshotGeneration, changeEvents, backendId);

    if (entry.hasChanges()) {
        feed.addEntry(entry);
    }
}
