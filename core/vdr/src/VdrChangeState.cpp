#include "VdrChangeState.h"

VdrChangeState::VdrChangeState()
    : statusVersion(0),
      channelsVersion(0),
      recordingsVersion(0),
      timersVersion(0),
      eventsVersion(0)
{
}

bool VdrChangeState::hasChangesComparedTo(const VdrChangeState& other) const
{
    return statusVersion != other.statusVersion
        || channelsVersion != other.channelsVersion
        || recordingsVersion != other.recordingsVersion
        || timersVersion != other.timersVersion
        || eventsVersion != other.eventsVersion;
}

bool VdrChangeState::isEmpty() const
{
    return statusVersion == 0
        && channelsVersion == 0
        && recordingsVersion == 0
        && timersVersion == 0
        && eventsVersion == 0;
}
