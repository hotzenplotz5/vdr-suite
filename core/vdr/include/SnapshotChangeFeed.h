#ifndef SNAPSHOT_CHANGE_FEED_H
#define SNAPSHOT_CHANGE_FEED_H

#include "SnapshotChangeFeedEntry.h"

#include <cstddef>
#include <vector>

class SnapshotChangeFeed {
public:
    static constexpr std::size_t maxRetainedEntries = 256;
    static constexpr std::size_t maxRetainedBytes = 512 * 1024;

    SnapshotChangeFeed();

    bool empty() const;
    int oldestSequenceNumber() const;
    int latestSequenceNumber() const;
    int latestSnapshotGeneration() const;
    const std::vector<SnapshotChangeFeedEntry>& entries() const;

    void addEntry(const SnapshotChangeFeedEntry& entry);
    void clear();

private:
    std::vector<SnapshotChangeFeedEntry> entries_;
    std::size_t retainedBytes_ = 0;
    int latestSequenceNumber_ = 0;
    int latestSnapshotGeneration_ = 0;
};

#endif
