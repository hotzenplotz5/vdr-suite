#include "VdrChangeState.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrChangeState empty;

    assert(empty.statusVersion == 0);
    assert(empty.channelsVersion == 0);
    assert(empty.recordingsVersion == 0);
    assert(empty.timersVersion == 0);
    assert(empty.eventsVersion == 0);
    assert(empty.isEmpty());

    VdrChangeState unchanged;
    assert(!empty.hasChangesComparedTo(unchanged));

    VdrChangeState changedStatus;
    changedStatus.statusVersion = 1;
    assert(changedStatus.hasChangesComparedTo(empty));
    assert(!changedStatus.isEmpty());

    VdrChangeState changedChannels;
    changedChannels.channelsVersion = 1;
    assert(changedChannels.hasChangesComparedTo(empty));

    VdrChangeState changedRecordings;
    changedRecordings.recordingsVersion = 1;
    assert(changedRecordings.hasChangesComparedTo(empty));

    VdrChangeState changedTimers;
    changedTimers.timersVersion = 1;
    assert(changedTimers.hasChangesComparedTo(empty));

    VdrChangeState changedEvents;
    changedEvents.eventsVersion = 1;
    assert(changedEvents.hasChangesComparedTo(empty));

    std::cout << "test_vdr_change_state passed" << std::endl;

    return 0;
}
