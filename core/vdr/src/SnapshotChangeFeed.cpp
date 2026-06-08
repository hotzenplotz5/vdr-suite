#include "SnapshotChangeFeed.h"

SnapshotChangeFeed::SnapshotChangeFeed()
{
}

bool SnapshotChangeFeed::empty() const
{
    return entries_.empty();
}

int SnapshotChangeFeed::latestSequenceNumber() const
{
    if (entries_.empty()) {
        return 0;
    }

    return entries_.back().sequenceNumber();
}

int SnapshotChangeFeed::latestSnapshotGeneration() const
{
    if (entries_.empty()) {
        return 0;
    }

    return entries_.back().snapshotGeneration();
}

const std::vector<SnapshotChangeFeedEntry>& SnapshotChangeFeed::entries() const
{
    return entries_;
}

void SnapshotChangeFeed::addEntry(const SnapshotChangeFeedEntry& entry)
{
    entries_.push_back(entry);
}

void SnapshotChangeFeed::clear()
{
    entries_.clear();
}
