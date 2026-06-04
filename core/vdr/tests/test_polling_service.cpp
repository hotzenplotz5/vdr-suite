#include "IVdrAdapter.h"
#include "PollingService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>
#include <vector>

class CountingVdrAdapter : public IVdrAdapter {
public:
    mutable int statusReadCount = 0;
    mutable int recordingsReadCount = 0;
    mutable int timersReadCount = 0;
    mutable int channelsReadCount = 0;
    mutable int eventsReadCount = 0;

    VdrChangeState changeState;

    VdrStatus getStatus() const override
    {
        ++statusReadCount;

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
        ++eventsReadCount;

        VdrEvent event;
        event.id = "event-1";

        return { event };
    }

    std::vector<VdrChannel> getChannels() const override
    {
        ++channelsReadCount;

        VdrChannel channel;
        channel.id = "channel-1";

        return { channel };
    }

    std::vector<VdrTimer> getTimers() const override
    {
        ++timersReadCount;

        VdrTimer timer;
        timer.id = "timer-1";

        return { timer };
    }

    std::vector<VdrRecording> getRecordings() const override
    {
        ++recordingsReadCount;

        VdrRecording recording;
        recording.id = "recording-1";

        return { recording };
    }

    VdrChangeState getChangeState() const override
    {
        return changeState;
    }

    int totalDomainReadCount() const
    {
        return statusReadCount
            + recordingsReadCount
            + timersReadCount
            + channelsReadCount
            + eventsReadCount;
    }
};

static PollingService createPollingService(
    VdrSnapshotBuilder& builder,
    VdrService& service,
    SnapshotCacheService& snapshotCacheService)
{
    return PollingService(builder, service, snapshotCacheService);
}

static void test_first_poll_builds_complete_snapshot_without_change_events()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService = createPollingService(builder, service, snapshotCacheService);

    pollingService.poll();

    assert(cache.hasSnapshot());
    assert(adapter.statusReadCount == 1);
    assert(adapter.recordingsReadCount == 1);
    assert(adapter.timersReadCount == 1);
    assert(adapter.channelsReadCount == 1);
    assert(adapter.eventsReadCount == 1);
    assert(adapter.totalDomainReadCount() == 5);

    assert(pollingService.snapshot().status.enabled == true);
    assert(pollingService.snapshot().recordings.size() == 1);
    assert(pollingService.snapshot().timers.size() == 1);
    assert(pollingService.snapshot().channels.size() == 1);
    assert(pollingService.snapshot().events.size() == 1);

    assert(pollingService.changeEvents().empty());
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == false);
}

static void test_unchanged_change_state_keeps_existing_snapshot_without_change_events()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService = createPollingService(builder, service, snapshotCacheService);

    adapter.changeState.channelsVersion = 1;

    pollingService.poll();
    pollingService.poll();

    assert(cache.hasSnapshot());
    assert(adapter.totalDomainReadCount() == 5);
    assert(pollingService.changeEvents().empty());
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == false);
}

static void test_channel_change_refreshes_only_channels_domain()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService = createPollingService(builder, service, snapshotCacheService);

    adapter.changeState.channelsVersion = 1;
    pollingService.poll();

    adapter.changeState.channelsVersion = 2;
    pollingService.poll();

    assert(cache.hasSnapshot());
    assert(adapter.statusReadCount == 1);
    assert(adapter.recordingsReadCount == 1);
    assert(adapter.timersReadCount == 1);
    assert(adapter.channelsReadCount == 2);
    assert(adapter.eventsReadCount == 1);
    assert(adapter.totalDomainReadCount() == 6);

    assert(pollingService.changeEvents().size() == 1);
    assert(pollingService.changeEvents()[0].type() == VdrChangeType::ChannelsChanged);
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == true);
    assert(pollingService.lastUpdatePlan().shouldRefreshChannels() == true);
    assert(pollingService.lastUpdatePlan().shouldRefreshRecordings() == false);
}

static void test_recording_change_refreshes_only_recordings_domain()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService = createPollingService(builder, service, snapshotCacheService);

    adapter.changeState.recordingsVersion = 1;
    pollingService.poll();

    adapter.changeState.recordingsVersion = 2;
    pollingService.poll();

    assert(adapter.statusReadCount == 1);
    assert(adapter.recordingsReadCount == 2);
    assert(adapter.timersReadCount == 1);
    assert(adapter.channelsReadCount == 1);
    assert(adapter.eventsReadCount == 1);
    assert(adapter.totalDomainReadCount() == 6);

    assert(pollingService.lastUpdatePlan().shouldRefreshRecordings() == true);
    assert(pollingService.lastUpdatePlan().shouldRefreshChannels() == false);
}

static void test_multiple_changes_refresh_only_selected_domains()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService = createPollingService(builder, service, snapshotCacheService);

    adapter.changeState.channelsVersion = 1;
    adapter.changeState.recordingsVersion = 1;
    pollingService.poll();

    adapter.changeState.channelsVersion = 2;
    adapter.changeState.recordingsVersion = 2;
    pollingService.poll();

    assert(pollingService.changeEvents().size() == 2);
    assert(pollingService.lastUpdatePlan().shouldRefreshChannels() == true);
    assert(pollingService.lastUpdatePlan().shouldRefreshRecordings() == true);
    assert(pollingService.lastUpdatePlan().shouldRefreshTimers() == false);
    assert(pollingService.lastUpdatePlan().shouldRefreshEvents() == false);

    assert(adapter.statusReadCount == 1);
    assert(adapter.recordingsReadCount == 2);
    assert(adapter.timersReadCount == 1);
    assert(adapter.channelsReadCount == 2);
    assert(adapter.eventsReadCount == 1);
    assert(adapter.totalDomainReadCount() == 7);
}

static void test_change_events_are_cleared_before_next_poll()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService = createPollingService(builder, service, snapshotCacheService);

    adapter.changeState.channelsVersion = 1;
    pollingService.poll();

    adapter.changeState.channelsVersion = 2;
    pollingService.poll();

    assert(pollingService.changeEvents().size() == 1);
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == true);

    pollingService.poll();

    assert(pollingService.changeEvents().empty());
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == false);
}

int main()
{
    test_first_poll_builds_complete_snapshot_without_change_events();
    test_unchanged_change_state_keeps_existing_snapshot_without_change_events();
    test_channel_change_refreshes_only_channels_domain();
    test_recording_change_refreshes_only_recordings_domain();
    test_multiple_changes_refresh_only_selected_domains();
    test_change_events_are_cleared_before_next_poll();

    std::cout << "test_polling_service passed" << std::endl;
    return 0;
}
