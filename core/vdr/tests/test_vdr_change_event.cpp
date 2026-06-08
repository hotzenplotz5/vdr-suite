#include "VdrChangeEvent.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrChangeEvent status(VdrChangeType::StatusChanged);
    assert(status.type() == VdrChangeType::StatusChanged);
    assert(status.typeName() == "StatusChanged");

    VdrChangeEvent channels(VdrChangeType::ChannelsChanged);
    assert(channels.typeName() == "ChannelsChanged");

    VdrChangeEvent recordings(VdrChangeType::RecordingsChanged);
    assert(recordings.typeName() == "RecordingsChanged");

    VdrChangeEvent timers(VdrChangeType::TimersChanged);
    assert(timers.typeName() == "TimersChanged");

    VdrChangeEvent events(VdrChangeType::EventsChanged);
    assert(events.typeName() == "EventsChanged");

    std::cout << "test_vdr_change_event passed" << std::endl;
    return 0;
}
