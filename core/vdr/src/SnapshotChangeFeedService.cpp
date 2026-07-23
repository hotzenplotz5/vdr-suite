#include "SnapshotChangeFeedService.h"

#include "SearchTimerPreviewEpgCacheChangeInvalidator.h"

#include <algorithm>

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
    case VdrChangeType::LiveOverlayChanged:
        return "liveOverlay";
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
        const std::string domain = domainNameForChangeType(event.type());

        if (!domain.empty() &&
            std::find(changedDomains.begin(), changedDomains.end(), domain) == changedDomains.end()) {
            changedDomains.push_back(domain);
        }

        if ((event.type() == VdrChangeType::StatusChanged ||
             event.type() == VdrChangeType::ChannelsChanged ||
             event.type() == VdrChangeType::TimersChanged ||
             event.type() == VdrChangeType::EventsChanged) &&
            std::find(changedDomains.begin(), changedDomains.end(), "liveOverlay") == changedDomains.end()) {
            changedDomains.push_back("liveOverlay");
        }
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
