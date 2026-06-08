#include "SnapshotChangeFeedEntry.h"

SnapshotChangeFeedEntry::SnapshotChangeFeedEntry(
    int sequenceNumber,
    int snapshotGeneration,
    const std::vector<std::string>& changedDomains)
    : sequenceNumber_(sequenceNumber),
      snapshotGeneration_(snapshotGeneration),
      changedDomains_(changedDomains)
{
}

int SnapshotChangeFeedEntry::sequenceNumber() const
{
    return sequenceNumber_;
}

int SnapshotChangeFeedEntry::snapshotGeneration() const
{
    return snapshotGeneration_;
}

const std::vector<std::string>& SnapshotChangeFeedEntry::changedDomains() const
{
    return changedDomains_;
}

bool SnapshotChangeFeedEntry::hasChanges() const
{
    return !changedDomains_.empty();
}
