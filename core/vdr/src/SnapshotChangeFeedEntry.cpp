#include "SnapshotChangeFeedEntry.h"

SnapshotChangeFeedEntry::SnapshotChangeFeedEntry(
    int sequenceNumber,
    int snapshotGeneration,
    const std::vector<std::string>& changedDomains,
    const std::string& backendId)
    : sequenceNumber_(sequenceNumber),
      snapshotGeneration_(snapshotGeneration),
      backendId_(backendId),
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

const std::string& SnapshotChangeFeedEntry::backendId() const
{
    return backendId_;
}

const std::vector<std::string>& SnapshotChangeFeedEntry::changedDomains() const
{
    return changedDomains_;
}

bool SnapshotChangeFeedEntry::hasChanges() const
{
    return !changedDomains_.empty();
}
