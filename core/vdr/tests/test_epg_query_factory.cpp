#include "EpgQueryFactory.h"

#include <cassert>
#include <iostream>

static void test_now_next_query_is_channel_scoped_and_limited()
{
    const VdrEventQuery query =
        EpgQueryFactory::forChannelNowNext(
            "C-61441-10023-10376",
            1780450000);

    assert(query.channelId == "C-61441-10023-10376");
    assert(query.eventId.empty() == true);
    assert(query.from == 1780450000);
    assert(query.timespan == 7200);
    assert(query.start == -1);
    assert(query.limit == 0);
    assert(query.channelEventLimit == 2);
    assert(query.onlyCount == false);
}

static void test_channel_time_window_query_is_channel_and_time_scoped()
{
    const VdrEventQuery query =
        EpgQueryFactory::forChannelTimeWindow(
            "C-61441-10023-10376",
            1780450000,
            14400);

    assert(query.channelId == "C-61441-10023-10376");
    assert(query.from == 1780450000);
    assert(query.timespan == 14400);
    assert(query.channelEventLimit == 0);
    assert(query.limit == 0);
}

static void test_channel_window_query_can_limit_result_count()
{
    const VdrEventQuery query =
        EpgQueryFactory::forChannelWindow(
            "C-61441-10023-10376",
            1780450000,
            14400,
            20);

    assert(query.channelId == "C-61441-10023-10376");
    assert(query.from == 1780450000);
    assert(query.timespan == 14400);
    assert(query.limit == 20);
}


static void test_create_now_next_request()
{
    EpgQueryRequest request;
    request.scope = EpgQueryScope::NowNext;
    request.channelId = "C-61441-10023-10376";
    request.from = 1780450000;

    const VdrEventQuery query = EpgQueryFactory::create(request);

    assert(query.channelId == "C-61441-10023-10376");
    assert(query.eventId.empty() == true);
    assert(query.from == 1780450000);
    assert(query.timespan == 7200);
    assert(query.start == -1);
    assert(query.limit == 0);
    assert(query.channelEventLimit == 2);
    assert(query.onlyCount == false);
}

static void test_create_time_window_request()
{
    EpgQueryRequest request;
    request.scope = EpgQueryScope::TimeWindow;
    request.channelId = "C-61441-10023-10376";
    request.from = 1780450000;
    request.timespan = 14400;

    const VdrEventQuery query = EpgQueryFactory::create(request);

    assert(query.channelId == "C-61441-10023-10376");
    assert(query.from == 1780450000);
    assert(query.timespan == 14400);
    assert(query.channelEventLimit == 0);
    assert(query.limit == 0);
}

static void test_create_channel_window_request()
{
    EpgQueryRequest request;
    request.scope = EpgQueryScope::ChannelWindow;
    request.channelId = "C-61441-10023-10376";
    request.from = 1780450000;
    request.timespan = 14400;
    request.limit = 20;

    const VdrEventQuery query = EpgQueryFactory::create(request);

    assert(query.channelId == "C-61441-10023-10376");
    assert(query.from == 1780450000);
    assert(query.timespan == 14400);
    assert(query.limit == 20);
}

int main()
{
    test_now_next_query_is_channel_scoped_and_limited();
    test_create_now_next_request();
    test_channel_time_window_query_is_channel_and_time_scoped();
    test_create_time_window_request();
    test_channel_window_query_can_limit_result_count();
    test_create_channel_window_request();

    std::cout << "test_epg_query_factory passed" << std::endl;
    return 0;
}
