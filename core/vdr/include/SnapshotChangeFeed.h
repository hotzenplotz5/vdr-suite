#ifndef SNAPSHOT_CHANGE_FEED_H
#define SNAPSHOT_CHANGE_FEED_H

#include "SnapshotChangeFeedEntry.h"

#include <vector>

class SnapshotChangeFeed {
public:
    SnapshotChangeFeed();

    bool empty() const;
    int latestSequenceNumber() const;
    int latestSnapshotGeneration() const;
    const std::vector<SnapshotChangeFeedEntry>& entries() const;

    void addEntry(const SnapshotChangeFeedEntry& entry);
    void clear();

private:
    std::vector<SnapshotChangeFeedEntry> entries_;
};

#endif
