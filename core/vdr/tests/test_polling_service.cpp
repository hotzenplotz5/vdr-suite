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

static bool containsMeasurement(const RecordingMeasurementSink& sink,const std::string& component,const std::string& operation)
{
    for (const auto& measurement : sink.measurements) {
        if (measurement.component == component && measurement.operation == operation) {
            return true;
        }
    }
    return false;
}

class CountingVdrAdapter : public IVdrAdapter {
public:
mutable int statusReadCount = 0; mutable int recordingsReadCount = 0; mutable int timersReadCount = 0; mutable int channelsReadCount = 0; mutable int eventsReadCount = 0; VdrChangeState changeState;
VdrStatus getStatus() const override { ++statusReadCount; VdrStatus s; s.enabled=true; s.mode="test"; s.host="test"; s.port=0; s.state="connected"; return s; }
std::vector<VdrEvent> getEvents() const override { ++eventsReadCount; VdrEvent e; e.id="event-1"; return {e}; }
std::vector<VdrChannel> getChannels() const override { ++channelsReadCount; VdrChannel c; c.id="channel-1"; return {c}; }
std::vector<VdrTimer> getTimers() const override { ++timersReadCount; VdrTimer t; t.id="timer-1"; return {t}; }
std::vector<VdrRecording> getRecordings() const override { ++recordingsReadCount; VdrRecording r; r.id="recording-1"; return {r}; }
VdrChangeState getChangeState() const override { return changeState; }
int totalDomainReadCount() const { return statusReadCount+recordingsReadCount+timersReadCount+channelsReadCount+eventsReadCount; }
};

static void test_polling_service_records_measurements()
{
    CountingVdrAdapter adapter;
    VdrService service(adapter);
    RecordingMeasurementSink sink;
    VdrSnapshotBuilder builder(service,nullptr,&sink);
    SnapshotCache cache;
    SnapshotCacheService snapshotCacheService(cache);
    PollingService pollingService(builder,service,snapshotCacheService,nullptr,&sink);

    adapter.changeState.channelsVersion = 1;
    pollingService.poll();
    adapter.changeState.channelsVersion = 2;
    pollingService.poll();

    assert(containsMeasurement(sink,"PollingService","Poll cycle"));
    assert(containsMeasurement(sink,"PollingService","Initial snapshot poll"));
    assert(containsMeasurement(sink,"PollingService","Detect changes"));
    assert(containsMeasurement(sink,"PollingService","Create update plan"));
    assert(containsMeasurement(sink,"PollingService","Channels refresh"));
    assert(containsMeasurement(sink,"PollingService","Partial refresh"));
}

int main()
{
    test_polling_service_records_measurements();
    std::cout << "test_polling_service passed" << std::endl;
    return 0;
}
