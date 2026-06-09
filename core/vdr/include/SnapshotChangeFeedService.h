#ifndef SNAPSHOT_CHANGE_FEED_SERVICE_H
#define SNAPSHOT_CHANGE_FEED_SERVICE_H

#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedEntry.h"
#include "VdrChangeEvent.h"

#include <vector>

class SnapshotChangeFeedService {
public:
    SnapshotChangeFeedEntry createEntry(
        int sequenceNumber,
        int snapshotGeneration,
        const std::vector<VdrChangeEvent>& changeEvents) const;

    SnapshotChangeFeed createFeed(
        int sequenceNumber,
        int snapshotGeneration,
        const std::vector<VdrChangeEvent>& changeEvents) const;

    void appendChanges(
        SnapshotChangeFeed& feed,
        int snapshotGeneration,
        const std::vector<VdrChangeEvent>& changeEvents) const;
};

#endif
