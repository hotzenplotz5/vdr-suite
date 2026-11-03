#include "LiveUpdateEvent.h"

LiveUpdateEvent::LiveUpdateEvent(
    int sequenceNumber,
    int snapshotGeneration,
    const std::vector<std::string>& changedDomains,
    const std::string& backendId,
    const std::vector<std::string>& recordingNativeIds)
    : sequenceNumber_(sequenceNumber),
      snapshotGeneration_(snapshotGeneration),
      backendId_(backendId),
      changedDomains_(changedDomains),
      recordingNativeIds_(recordingNativeIds)
{
}

LiveUpdateEvent::LiveUpdateEvent(
    const SnapshotChangeFeedEntry& entry)
    : LiveUpdateEvent(
          entry.sequenceNumber(),
          entry.snapshotGeneration(),
          entry.changedDomains(),
          entry.backendId(),
          entry.recordingNativeIds())
{
}

int LiveUpdateEvent::sequenceNumber() const
{
    return sequenceNumber_;
}

int LiveUpdateEvent::snapshotGeneration() const
{
    return snapshotGeneration_;
}

const std::string& LiveUpdateEvent::backendId() const
{
    return backendId_;
}

const std::vector<std::string>& LiveUpdateEvent::changedDomains() const
{
    return changedDomains_;
}

const std::vector<std::string>& LiveUpdateEvent::recordingNativeIds() const
{
    return recordingNativeIds_;
}

bool LiveUpdateEvent::hasChanges() const
{
    return !changedDomains_.empty();
}
