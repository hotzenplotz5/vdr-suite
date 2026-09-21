#include "IVdrAdapter.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>
#include <vector>

class StartupSnapshotCountingAdapter final : public IVdrAdapter
{
public:
    mutable int statusReadCount = 0;
    mutable int recordingsReadCount = 0;
    mutable int timersReadCount = 0;
    mutable int channelsReadCount = 0;
    mutable int eventsReadCount = 0;
    mutable int selectiveEventsReadCount = 0;

    VdrStatus getStatus() const override
    {
        ++statusReadCount;

        VdrStatus status;
        status.enabled = true;
        status.mode = "startup-test";
        status.state = "connected";
        return status;
    }

    std::vector<VdrEvent> getEvents() const override
    {
        ++eventsReadCount;

        VdrEvent event;
        event.id = "event-1";
        return {event};
    }

    std::vector<VdrEvent> getEvents(const VdrEventQuery&) const override
    {
        ++selectiveEventsReadCount;

        VdrEvent event;
        event.id = "selective-event-1";
        return {event};
    }

    std::vector<VdrChannel> getChannels() const override
    {
        ++channelsReadCount;

        VdrChannel channel;
        channel.id = "channel-1";
        return {channel};
    }

    std::vector<VdrTimer> getTimers() const override
    {
        ++timersReadCount;

        VdrTimer timer;
        timer.id = "timer-1";
        return {timer};
    }

    std::vector<VdrRecording> getRecordings() const override
    {
        ++recordingsReadCount;

        VdrRecording recording;
        recording.id = "recording-1";
        return {recording};
    }

    VdrChangeState getChangeState() const override
    {
        return VdrChangeState();
    }
};

int main()
{
    StartupSnapshotCountingAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(
        service,
        "default",
        nullptr,
        nullptr);

    const VdrSnapshot snapshot = builder.buildStartupSnapshot();

    assert(snapshot.backendId == "default");
    assert(snapshot.status.enabled == true);
    assert(snapshot.status.mode == "startup-test");

    assert(snapshot.channels.size() == 1);
    assert(snapshot.timers.size() == 1);
    assert(snapshot.searchTimers.empty());
    assert(snapshot.recordings.empty());
    assert(snapshot.events.empty());

    assert(adapter.statusReadCount == 1);
    assert(adapter.channelsReadCount == 1);
    assert(adapter.timersReadCount == 1);
    assert(adapter.recordingsReadCount == 0);
    assert(adapter.eventsReadCount == 0);
    assert(adapter.selectiveEventsReadCount == 0);

    std::cout << "test_vdr_snapshot_builder_startup_snapshot passed" << std::endl;
    return 0;
}
