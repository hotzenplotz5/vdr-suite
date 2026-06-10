#include "IRuntimeMeasurementSink.h"
#include "IVdrAdapter.h"
#include "PollingService.h"
#include "RuntimeMeasurement.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class RecordingMeasurementSink : public IRuntimeMeasurementSink {
public:
    void recordMeasurement(const RuntimeMeasurement& measurement) override
    {
        measurements.push_back(measurement);
    }

    std::vector<RuntimeMeasurement> measurements;
};

static bool containsMeasurement(
    const RecordingMeasurementSink& sink,
    const std::string& component,
    const std::string& operation)
{
    for (const RuntimeMeasurement& measurement : sink.measurements) {
        if (measurement.component == component && measurement.operation == operation) {
            return true;
        }
    }

    return false;
}

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

static void test_first_poll_records_initial_poll_measurements()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService(builder, service, snapshotCacheService, nullptr, &sink);

    pollingService.poll();

    assert(cache.hasSnapshot());
    assert(containsMeasurement(sink, "PollingService", "Initial snapshot poll"));
    assert(containsMeasurement(sink, "PollingService", "Poll cycle"));
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

static void test_unchanged_change_state_records_detection_and_plan_measurements()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService(builder, service, snapshotCacheService, nullptr, &sink);

    adapter.changeState.channelsVersion = 1;

    pollingService.poll();
    pollingService.poll();

    assert(containsMeasurement(sink, "PollingService", "Detect changes"));
    assert(containsMeasurement(sink, "PollingService", "Create update plan"));
    assert(containsMeasurement(sink, "PollingService", "Poll cycle"));
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

static void test_channel_change_records_channel_refresh_measurements()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService(builder, service, snapshotCacheService, nullptr, &sink);

    adapter.changeState.channelsVersion = 1;
    pollingService.poll();

    adapter.changeState.channelsVersion = 2;
    pollingService.poll();

    assert(containsMeasurement(sink, "PollingService", "Channels refresh"));
    assert(containsMeasurement(sink, "PollingService", "Partial refresh"));
    assert(containsMeasurement(sink, "PollingService", "Poll cycle"));
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

static void test_recording_change_records_recordings_refresh_measurements()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService(builder, service, snapshotCacheService, nullptr, &sink);

    adapter.changeState.recordingsVersion = 1;
    pollingService.poll();

    adapter.changeState.recordingsVersion = 2;
    pollingService.poll();

    assert(containsMeasurement(sink, "PollingService", "Recordings refresh"));
    assert(containsMeasurement(sink, "PollingService", "Partial refresh"));
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

static void test_polling_service_updates_backend_snapshot()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(
        service,
        "parents-vdr",
        nullptr,
        nullptr);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);

    PollingService pollingService(
        builder,
        service,
        snapshotCacheService,
        "parents-vdr");

    pollingService.poll();

    assert(cache.hasSnapshotForBackend("parents-vdr"));

    const VdrSnapshot* snapshot =
        cache.snapshotForBackend("parents-vdr");

    assert(snapshot != nullptr);
    assert(snapshot->backendId == "parents-vdr");
    assert(snapshot->channels.size() == 1);
    assert(snapshot->recordings.size() == 1);
}

int main()
{
    test_first_poll_builds_complete_snapshot_without_change_events();
    test_first_poll_records_initial_poll_measurements();
    test_unchanged_change_state_keeps_existing_snapshot_without_change_events();
    test_unchanged_change_state_records_detection_and_plan_measurements();
    test_channel_change_refreshes_only_channels_domain();
    test_channel_change_records_channel_refresh_measurements();
    test_recording_change_refreshes_only_recordings_domain();
    test_recording_change_records_recordings_refresh_measurements();
    test_multiple_changes_refresh_only_selected_domains();
    test_change_events_are_cleared_before_next_poll();
    test_polling_service_updates_backend_snapshot();

    std::cout << "test_polling_service passed" << std::endl;
    return 0;
}
