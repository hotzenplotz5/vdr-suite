#include "MockVdrAdapter.h"
#include "IVdrAdapter.h"

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

int main()
{
    test_mock_vdr_adapter_reports_connected_state();
    test_mock_vdr_adapter_can_be_used_through_interface();
    test_mock_vdr_adapter_returns_events();

    return 0;
}
