#include "MockVdrAdapter.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrService.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <cassert>
#include <vector>

static void test_vdr_service_returns_status_from_adapter()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);

    VdrStatus status = service.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "mock");
    assert(status.host == "mock");
    assert(status.port == 0);
    assert(status.state == "connected");
}

static void test_vdr_service_returns_channels_from_adapter()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);

    std::vector<VdrChannel> channels = service.getChannels();

    assert(channels.size() == 3);
    assert(channels[0].id == "mock-channel-1");
    assert(channels[0].number == 1);
    assert(channels[0].name == "Das Erste HD");
    assert(channels[2].radio == true);
}

static void test_vdr_service_returns_events_from_adapter()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);

    std::vector<VdrEvent> events = service.getEvents();

    assert(events.size() == 2);
    assert(events[0].id == "mock-event-1");
    assert(events[0].channelId == "mock-channel-1");
    assert(events[0].title == "Tagesschau");
    assert(events[1].title == "Tatort");
    assert(events[1].parentalRating == 12);
}

static void test_vdr_service_returns_filtered_events_from_adapter()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);

    VdrEventQuery query;
    query.channelId = "mock-channel-1";
    query.eventId = "mock-event-2";

    std::vector<VdrEvent> events = service.getEvents(query);

    assert(events.size() == 1);
    assert(events[0].id == "mock-event-2");
    assert(events[0].title == "Tatort");
}

static void test_vdr_service_returns_timers_from_adapter()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);

    std::vector<VdrTimer> timers = service.getTimers();

    assert(timers.size() == 1);
    assert(timers[0].id == "mock-timer-1");
    assert(timers[0].channelId == "mock-channel-1");
    assert(timers[0].eventId == "mock-event-1");
    assert(timers[0].enabled == true);
    assert(timers[0].recording == false);
}

static void test_vdr_service_returns_recordings_from_adapter()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);

    std::vector<VdrRecording> recordings = service.getRecordings();

    assert(recordings.size() == 2);
    assert(recordings[0].id == "mock-recording-1");
    assert(recordings[0].title == "Tagesschau");
    assert(recordings[1].id == "mock-recording-2");
    assert(recordings[1].title == "Tatort");
}

int main()
{
    test_vdr_service_returns_status_from_adapter();
    test_vdr_service_returns_channels_from_adapter();
    test_vdr_service_returns_events_from_adapter();
    test_vdr_service_returns_filtered_events_from_adapter();
    test_vdr_service_returns_timers_from_adapter();
    test_vdr_service_returns_recordings_from_adapter();

    return 0;
}
