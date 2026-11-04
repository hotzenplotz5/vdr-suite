#include "SnapshotChangeFeed.h"

namespace {
std::size_t retainedSize(const SnapshotChangeFeedEntry& entry)
{
    const std::size_t limit = SnapshotChangeFeed::maxRetainedBytes;
    std::size_t bytes = sizeof(SnapshotChangeFeedEntry);
    const auto addString = [&](const std::string& value) {
        if (bytes > limit || value.size() > limit - bytes)
            bytes = limit + 1;
        else
            bytes += value.size();
    };
    addString(entry.backendId());
    for (const auto& domain : entry.changedDomains()) addString(domain);
    for (const auto& id : entry.recordingNativeIds()) addString(id);
    return bytes;
}
}

SnapshotChangeFeed::SnapshotChangeFeed() {}

bool SnapshotChangeFeed::empty() const
{
    return entries_.empty();
}

int SnapshotChangeFeed::oldestSequenceNumber() const
{
    return entries_.empty() ? 0 : entries_.front().sequenceNumber();
}

int SnapshotChangeFeed::latestSequenceNumber() const
{
    return latestSequenceNumber_;
}

int SnapshotChangeFeed::latestSnapshotGeneration() const
{
    return latestSnapshotGeneration_;
}

const std::vector<SnapshotChangeFeedEntry>& SnapshotChangeFeed::entries() const
{
    return entries_;
}

void SnapshotChangeFeed::addEntry(const SnapshotChangeFeedEntry& entry)
{
    if (entry.sequenceNumber() <= latestSequenceNumber_) return;
    latestSequenceNumber_ = entry.sequenceNumber();
    latestSnapshotGeneration_ = entry.snapshotGeneration();
    const std::size_t bytes = retainedSize(entry);
    if (bytes > maxRetainedBytes) {
        entries_.clear();
        retainedBytes_ = 0;
        return;
    }
    while (!entries_.empty() &&
           (entries_.size() >= maxRetainedEntries ||
            retainedBytes_ > maxRetainedBytes - bytes)) {
        retainedBytes_ -= retainedSize(entries_.front());
        entries_.erase(entries_.begin());
    }
    entries_.push_back(entry);
    retainedBytes_ += bytes;
}

void SnapshotChangeFeed::clear()
{
    entries_.clear();
    retainedBytes_ = 0;
    // Preserve the sequence high-water mark within one daemon epoch.
}
