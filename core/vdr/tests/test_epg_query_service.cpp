#include "EpgQueryService.h"
#include "MockVdrAdapter.h"
#include "VdrService.h"

#include <cassert>
#include <iostream>

static void test_now_next_returns_channel_scoped_events()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    EpgQueryService epgService(vdrService);

    const auto events =
        epgService.getNowNext("mock-channel-1", 1780450000);

    assert(events.size() == 2);
    assert(events[0].id == "mock-event-1");
    assert(events[1].id == "mock-event-2");
}

static void test_time_window_returns_channel_scoped_events()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    EpgQueryService epgService(vdrService);

    const auto events =
        epgService.getTimeWindow("mock-channel-1", 1780450000, 14400);

    assert(events.size() == 2);
    assert(events[0].channelId == "mock-channel-1");
    assert(events[1].channelId == "mock-channel-1");
}

static void test_unknown_channel_returns_empty_events()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    EpgQueryService epgService(vdrService);

    const auto events =
        epgService.getChannelWindow("unknown-channel", 1780450000, 14400, 20);

    assert(events.empty() == true);
}

int main()
{
    test_now_next_returns_channel_scoped_events();
    test_time_window_returns_channel_scoped_events();
    test_unknown_channel_returns_empty_events();

    std::cout << "test_epg_query_service passed" << std::endl;
    return 0;
}
