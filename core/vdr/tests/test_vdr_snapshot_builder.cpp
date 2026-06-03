#include "MockVdrAdapter.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>

static void test_snapshot_builder_collects_complete_vdr_state()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);

    VdrSnapshot snapshot = builder.buildSnapshot();

    assert(snapshot.status.enabled == true);
    assert(snapshot.status.mode == "mock");
    assert(snapshot.status.state == "connected");

    assert(snapshot.recordings.size() == 2);
    assert(snapshot.recordings[0].id == "mock-recording-1");

    assert(snapshot.timers.size() == 2);
    assert(snapshot.timers[0].id == "mock-timer-1");

    assert(snapshot.channels.size() == 3);
    assert(snapshot.channels[0].id == "mock-channel-1");

    assert(snapshot.events.size() == 2);
    assert(snapshot.events[0].id == "mock-event-1");
}

int main()
{
    test_snapshot_builder_collects_complete_vdr_state();

    std::cout
        << "test_vdr_snapshot_builder passed"
        << std::endl;

    return 0;
}
