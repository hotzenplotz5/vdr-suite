#include "MockVdrAdapter.h"
#include "IVdrAdapter.h"
#include "VdrChannel.h"
#include "VdrRecording.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

static void test_mock_vdr_adapter_reports_connected_state()
{
    MockVdrAdapter adapter;
    VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "mock");
    assert(status.host == "mock");
    assert(status.port == 0);
    assert(status.state == "connected");
}

static void test_mock_vdr_adapter_can_be_used_through_interface()
{
    std::unique_ptr<IVdrAdapter> adapter =
        std::make_unique<MockVdrAdapter>();

    VdrStatus status = adapter->getStatus();

    assert(status.enabled == true);
    assert(status.mode == "mock");
    assert(status.state == "connected");
}

static void test_mock_vdr_adapter_returns_channels()
{
    MockVdrAdapter adapter;
    std::vector<VdrChannel> channels = adapter.getChannels();

    assert(channels.size() == 3);

    assert(channels[0].id == "mock-channel-1");
    assert(channels[0].number == 1);
    assert(channels[0].name == "Das Erste HD");
    assert(channels[0].provider == "ARD");
    assert(channels[0].group == "TV");
    assert(channels[0].radio == false);
    assert(channels[0].encrypted == false);
    assert(channels[0].enabled == true);

    assert(channels[2].id == "mock-channel-3");
    assert(channels[2].number == 101);
    assert(channels[2].name == "Radio Hamburg");
    assert(channels[2].radio == true);
}

static void test_mock_vdr_adapter_returns_channels_through_interface()
{
    std::unique_ptr<IVdrAdapter> adapter =
        std::make_unique<MockVdrAdapter>();

    std::vector<VdrChannel> channels = adapter->getChannels();

    assert(channels.size() == 3);
    assert(channels[0].id == "mock-channel-1");
}

static void test_mock_vdr_adapter_returns_events()
{
    MockVdrAdapter adapter;
    std::vector<VdrEvent> events = adapter.getEvents();

    assert(events.size() == 2);

    assert(events[0].id == "mock-event-1");
    assert(events[0].channelId == "mock-channel-1");
    assert(events[0].title == "Tagesschau");
    assert(events[0].subtitle == "20 Uhr");
    assert(events[0].description == "Nachrichten des Tages");
    assert(events[0].startTime == "2026-06-01T20:00:00");
    assert(events[0].endTime == "2026-06-01T20:15:00");
    assert(events[0].durationSeconds == 900);
    assert(events[0].contentDescriptors.size() == 1);
    assert(events[0].contentDescriptors[0] == "news");
    assert(events[0].parentalRating == 0);

    assert(events[1].id == "mock-event-2");
    assert(events[1].channelId == "mock-channel-1");
    assert(events[1].title == "Tatort");
    assert(events[1].subtitle == "Borowski und das Meer");
    assert(events[1].description == "Kriminalfilm");
    assert(events[1].startTime == "2026-06-01T20:15:00");
    assert(events[1].endTime == "2026-06-01T21:45:00");
    assert(events[1].durationSeconds == 5400);
    assert(events[1].contentDescriptors.size() == 2);
    assert(events[1].contentDescriptors[0] == "movie");
    assert(events[1].contentDescriptors[1] == "crime");
    assert(events[1].parentalRating == 12);
}

static void test_mock_vdr_adapter_returns_recordings()
{
    MockVdrAdapter adapter;
    std::vector<VdrRecording> recordings = adapter.getRecordings();

    assert(recordings.size() == 2);

    assert(recordings[0].id == "mock-recording-1");
    assert(recordings[0].title == "Tagesschau");
    assert(recordings[0].path == "/Mock/Tagesschau/2026-06-01.20.00.1-0.rec");
    assert(recordings[0].startTime == "2026-06-01T20:00:00");
    assert(recordings[0].durationSeconds == 900);
    assert(recordings[0].sizeMb == 512);

    assert(recordings[1].id == "mock-recording-2");
    assert(recordings[1].title == "Tatort");
    assert(recordings[1].durationSeconds == 5400);
    assert(recordings[1].sizeMb == 4096);
}

static void test_mock_vdr_adapter_returns_recordings_through_interface()
{
    std::unique_ptr<IVdrAdapter> adapter =
        std::make_unique<MockVdrAdapter>();

    std::vector<VdrRecording> recordings = adapter->getRecordings();

    assert(recordings.size() == 2);
    assert(recordings[0].id == "mock-recording-1");
}

int main()
{
    test_mock_vdr_adapter_reports_connected_state();
    test_mock_vdr_adapter_can_be_used_through_interface();
    test_mock_vdr_adapter_returns_channels();
    test_mock_vdr_adapter_returns_channels_through_interface();
    test_mock_vdr_adapter_returns_events();
    test_mock_vdr_adapter_returns_recordings();
    test_mock_vdr_adapter_returns_recordings_through_interface();

    return 0;
}
