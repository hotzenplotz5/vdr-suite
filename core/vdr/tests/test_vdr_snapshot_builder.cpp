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

    assert(snapshot.timers.size() == 1);
    assert(snapshot.timers[0].id == "mock-timer-1");

    assert(snapshot.channels.size() == 3);
    assert(snapshot.channels[0].id == "mock-channel-1");

    assert(snapshot.events.size() == 2);
    assert(snapshot.events[0].id == "mock-event-1");
}

static void test_snapshot_builder_can_build_status_domain()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);

    VdrStatus status = builder.buildStatus();

    assert(status.enabled == true);
    assert(status.mode == "mock");
    assert(status.state == "connected");
}

static void test_snapshot_builder_can_build_recordings_domain()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);

    auto recordings = builder.buildRecordings();

    assert(recordings.size() == 2);
    assert(recordings[0].id == "mock-recording-1");
}

static void test_snapshot_builder_can_build_timers_domain()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);

    auto timers = builder.buildTimers();

    assert(timers.size() == 1);
    assert(timers[0].id == "mock-timer-1");
}

static void test_snapshot_builder_can_build_channels_domain()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);

    auto channels = builder.buildChannels();

    assert(channels.size() == 3);
    assert(channels[0].id == "mock-channel-1");
}

static void test_snapshot_builder_can_build_events_domain()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);

    auto events = builder.buildEvents();

    assert(events.size() == 2);
    assert(events[0].id == "mock-event-1");
}

int main()
{
    test_snapshot_builder_collects_complete_vdr_state();
    test_snapshot_builder_can_build_status_domain();
    test_snapshot_builder_can_build_recordings_domain();
    test_snapshot_builder_can_build_timers_domain();
    test_snapshot_builder_can_build_channels_domain();
    test_snapshot_builder_can_build_events_domain();

    std::cout
        << "test_vdr_snapshot_builder passed"
        << std::endl;

    return 0;
}
