#include "IVdrAdapter.h"
#include "PollingService.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>
#include <vector>

class CountingVdrAdapter : public IVdrAdapter {
public:
    mutable int snapshotReadCount = 0;
    VdrChangeState changeState;

    VdrStatus getStatus() const override
    {
        ++snapshotReadCount;

        VdrStatus status;
        status.enabled = true;
        status.mode = "test";
        status.host = "test";
        status.port = 0;
        status.state = "connected";

        return status;
    }

    std::vector<VdrEvent> getEvents() const override
    {
        return {};
    }

    std::vector<VdrChannel> getChannels() const override
    {
        return {};
    }

    std::vector<VdrTimer> getTimers() const override
    {
        return {};
    }

    std::vector<VdrRecording> getRecordings() const override
    {
        return {};
    }

    VdrChangeState getChangeState() const override
    {
        return changeState;
    }
};

static void test_first_poll_builds_snapshot_without_change_events()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    PollingService pollingService(builder, service);

    pollingService.poll();

    assert(adapter.snapshotReadCount == 1);
    assert(pollingService.snapshot().status.enabled == true);
    assert(pollingService.changeEvents().empty());
}

static void test_unchanged_change_state_keeps_existing_snapshot_without_change_events()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    PollingService pollingService(builder, service);

    adapter.changeState.channelsVersion = 1;

    pollingService.poll();
    pollingService.poll();

    assert(adapter.snapshotReadCount == 1);
    assert(pollingService.changeEvents().empty());
}

static void test_changed_change_state_refreshes_snapshot_and_exposes_change_events()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    PollingService pollingService(builder, service);

    adapter.changeState.channelsVersion = 1;
    pollingService.poll();

    adapter.changeState.channelsVersion = 2;
    pollingService.poll();

    assert(adapter.snapshotReadCount == 2);
    assert(pollingService.changeEvents().size() == 1);
    assert(pollingService.changeEvents()[0].type() == VdrChangeType::ChannelsChanged);
}

static void test_change_events_are_cleared_before_next_poll()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    PollingService pollingService(builder, service);

    adapter.changeState.channelsVersion = 1;
    pollingService.poll();

    adapter.changeState.channelsVersion = 2;
    pollingService.poll();

    assert(pollingService.changeEvents().size() == 1);

    pollingService.poll();

    assert(pollingService.changeEvents().empty());
}

int main()
{
    test_first_poll_builds_snapshot_without_change_events();
    test_unchanged_change_state_keeps_existing_snapshot_without_change_events();
    test_changed_change_state_refreshes_snapshot_and_exposes_change_events();
    test_change_events_are_cleared_before_next_poll();

    std::cout << "test_polling_service passed" << std::endl;
    return 0;
}
