#include "IRuntimeMeasurementSink.h"
#include "MockVdrAdapter.h"
#include "RuntimeMeasurement.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
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

static const RuntimeMeasurement& findMeasurement(
    const RecordingMeasurementSink& sink,
    const std::string& component,
    const std::string& operation)
{
    for (const RuntimeMeasurement& measurement : sink.measurements) {
        if (measurement.component == component && measurement.operation == operation) {
            return measurement;
        }
    }

    assert(false);
    return sink.measurements.front();
}

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

static void test_snapshot_builder_records_measurements_for_complete_snapshot()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);

    VdrSnapshot snapshot = builder.buildSnapshot();

    assert(snapshot.status.enabled == true);
    assert(snapshot.recordings.size() == 2);
    assert(snapshot.timers.size() == 1);
    assert(snapshot.channels.size() == 3);
    assert(snapshot.events.size() == 2);

    assert(sink.measurements.size() == 5);
    assert(containsMeasurement(sink, "VdrSnapshotBuilder", "Build status"));
    assert(containsMeasurement(sink, "VdrSnapshotBuilder", "Build recordings"));
    assert(containsMeasurement(sink, "VdrSnapshotBuilder", "Build timers"));
    assert(containsMeasurement(sink, "VdrSnapshotBuilder", "Build channels"));
    assert(containsMeasurement(sink, "VdrSnapshotBuilder", "Build events"));

    assert(findMeasurement(sink, "VdrSnapshotBuilder", "Build recordings").itemCount == 2);
    assert(findMeasurement(sink, "VdrSnapshotBuilder", "Build timers").itemCount == 1);
    assert(findMeasurement(sink, "VdrSnapshotBuilder", "Build channels").itemCount == 3);
    assert(findMeasurement(sink, "VdrSnapshotBuilder", "Build events").itemCount == 2);
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

static void test_snapshot_builder_records_status_measurement()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);

    VdrStatus status = builder.buildStatus();

    assert(status.enabled == true);
    assert(sink.measurements.size() == 1);
    assert(sink.measurements[0].component == "VdrSnapshotBuilder");
    assert(sink.measurements[0].operation == "Build status");
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

static void test_snapshot_builder_records_recordings_measurement()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);

    auto recordings = builder.buildRecordings();

    assert(recordings.size() == 2);
    assert(sink.measurements.size() == 1);
    assert(sink.measurements[0].component == "VdrSnapshotBuilder");
    assert(sink.measurements[0].operation == "Build recordings");
    assert(sink.measurements[0].itemCount == 2);
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

static void test_snapshot_builder_records_timers_measurement()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);

    auto timers = builder.buildTimers();

    assert(timers.size() == 1);
    assert(sink.measurements.size() == 1);
    assert(sink.measurements[0].component == "VdrSnapshotBuilder");
    assert(sink.measurements[0].operation == "Build timers");
    assert(sink.measurements[0].itemCount == 1);
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

static void test_snapshot_builder_records_channels_measurement()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);

    auto channels = builder.buildChannels();

    assert(channels.size() == 3);
    assert(sink.measurements.size() == 1);
    assert(sink.measurements[0].component == "VdrSnapshotBuilder");
    assert(sink.measurements[0].operation == "Build channels");
    assert(sink.measurements[0].itemCount == 3);
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

static void test_snapshot_builder_records_events_measurement()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);

    auto events = builder.buildEvents();

    assert(events.size() == 2);
    assert(sink.measurements.size() == 1);
    assert(sink.measurements[0].component == "VdrSnapshotBuilder");
    assert(sink.measurements[0].operation == "Build events");
    assert(sink.measurements[0].itemCount == 2);
}

static void test_snapshot_builder_can_build_selective_events_domain()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);

    VdrEventQuery query;
    query.channelEventLimit = 1;

    auto events = builder.buildEvents(query);

    assert(events.size() == 2);
    assert(events[0].id == "mock-event-1");
}

static void test_snapshot_builder_records_selective_events_measurement()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service, nullptr, &sink);

    VdrEventQuery query;
    query.channelEventLimit = 1;

    auto events = builder.buildEvents(query);

    assert(events.size() == 2);
    assert(sink.measurements.size() == 1);
    assert(sink.measurements[0].component == "VdrSnapshotBuilder");
    assert(sink.measurements[0].operation == "Build selective events");
    assert(sink.measurements[0].itemCount == 2);
}

static void test_snapshot_builder_assigns_backend_id()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);

    VdrSnapshotBuilder builder(
        service,
        "ferienhaus",
        nullptr,
        nullptr);

    VdrSnapshot snapshot =
        builder.buildSnapshot();

    assert(snapshot.backendId == "ferienhaus");
}

int main()
{
    test_snapshot_builder_collects_complete_vdr_state();
    test_snapshot_builder_records_measurements_for_complete_snapshot();
    test_snapshot_builder_can_build_status_domain();
    test_snapshot_builder_records_status_measurement();
    test_snapshot_builder_can_build_recordings_domain();
    test_snapshot_builder_records_recordings_measurement();
    test_snapshot_builder_can_build_timers_domain();
    test_snapshot_builder_records_timers_measurement();
    test_snapshot_builder_can_build_channels_domain();
    test_snapshot_builder_records_channels_measurement();
    test_snapshot_builder_can_build_events_domain();
    test_snapshot_builder_records_events_measurement();
    test_snapshot_builder_can_build_selective_events_domain();
    test_snapshot_builder_records_selective_events_measurement();
    test_snapshot_builder_assigns_backend_id();

    std::cout
        << "test_vdr_snapshot_builder passed"
        << std::endl;

    return 0;
}
