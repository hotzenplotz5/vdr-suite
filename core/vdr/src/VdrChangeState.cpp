#include "VdrChangeState.h"

VdrChangeState::VdrChangeState()
    : bootId(""),
      statusVersion(0),
      channelsVersion(0),
      recordingsVersion(0),
      timersVersion(0),
      searchTimersVersion(0),
      eventsVersion(0)
{
}

bool VdrChangeState::hasChangesComparedTo(const VdrChangeState& other) const
{
    return bootId != other.bootId
        || statusVersion != other.statusVersion
        || channelsVersion != other.channelsVersion
        || recordingsVersion != other.recordingsVersion
        || timersVersion != other.timersVersion
        || searchTimersVersion != other.searchTimersVersion
        || eventsVersion != other.eventsVersion;
}

bool VdrChangeState::isEmpty() const
{
    return bootId.empty()
        && statusVersion == 0
        && channelsVersion == 0
        && recordingsVersion == 0
        && timersVersion == 0
        && searchTimersVersion == 0
        && eventsVersion == 0;
}
