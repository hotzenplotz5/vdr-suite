#include "EpgController.h"
#include "IEpgQueryService.h"
#include "VdrEvent.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class TestEpgQueryService : public IEpgQueryService
{
public:
    std::vector<VdrEvent> getNowNext(
        const std::string& channelId,
        int from) const override
    {
        lastChannelId = channelId;
        lastFrom = from;
        lastCall = 1;
        return makeEvents("event-now");
    }

    std::vector<VdrEvent> getTimeWindow(
        const std::string& channelId,
        int from,
        int timespan) const override
    {
        lastChannelId = channelId;
        lastFrom = from;
        lastTimespan = timespan;
        lastCall = 2;
        return makeEvents("event-time");
    }

    std::vector<VdrEvent> getChannelWindow(
        const std::string& channelId,
        int from,
        int timespan,
        int limit) const override
    {
        lastChannelId = channelId;
        lastFrom = from;
        lastTimespan = timespan;
        lastLimit = limit;
        lastCall = 3;
        return makeEvents("event-channel");
    }

    mutable int lastCall = 0;
    mutable std::string lastChannelId;
    mutable int lastFrom = 0;
    mutable int lastTimespan = 0;
    mutable int lastLimit = 0;

private:
    static std::vector<VdrEvent> makeEvents(const std::string& id)
    {
        VdrEvent event;
        event.id = id;
        event.channelId = "channel-1";
        event.title = "Title";
        event.startTime = "2026-06-13T20:00:00";
        event.endTime = "2026-06-13T21:00:00";
        event.durationSeconds = 3600;

        return {event};
    }
};

static void assertEventResponse(
    const ApiResponse& response,
    const std::string& expectedId)
{
    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find(expectedId) != std::string::npos);
    assert(response.body.find("events") != std::string::npos);
    assert(response.body.find("count") != std::string::npos);
}

int main()
{
    TestEpgQueryService epgQueryService;
    EpgController controller(epgQueryService);

    ApiResponse nowNextResponse = controller.getNowNext();
    assertEventResponse(nowNextResponse, "event-now");
    assert(epgQueryService.lastCall == 1);
    assert(epgQueryService.lastFrom == -1);

    ApiResponse nowNextParameterizedResponse =
        controller.getNowNext("channel-42", 123);
    assertEventResponse(nowNextParameterizedResponse, "event-now");
    assert(epgQueryService.lastCall == 1);
    assert(epgQueryService.lastChannelId == "channel-42");
    assert(epgQueryService.lastFrom == 123);

    ApiResponse timeWindowResponse = controller.getTimeWindow();
    assertEventResponse(timeWindowResponse, "event-time");
    assert(epgQueryService.lastCall == 2);
    assert(epgQueryService.lastTimespan == 7200);

    ApiResponse timeWindowParameterizedResponse =
        controller.getTimeWindow("channel-43", 456, 3600);
    assertEventResponse(timeWindowParameterizedResponse, "event-time");
    assert(epgQueryService.lastCall == 2);
    assert(epgQueryService.lastChannelId == "channel-43");
    assert(epgQueryService.lastFrom == 456);
    assert(epgQueryService.lastTimespan == 3600);

    ApiResponse channelWindowResponse = controller.getChannelWindow();
    assertEventResponse(channelWindowResponse, "event-channel");
    assert(epgQueryService.lastCall == 3);
    assert(epgQueryService.lastLimit == 5);

    ApiResponse channelWindowParameterizedResponse =
        controller.getChannelWindow("channel-44", 789, 1800, 10);
    assertEventResponse(channelWindowParameterizedResponse, "event-channel");
    assert(epgQueryService.lastCall == 3);
    assert(epgQueryService.lastChannelId == "channel-44");
    assert(epgQueryService.lastFrom == 789);
    assert(epgQueryService.lastTimespan == 1800);
    assert(epgQueryService.lastLimit == 10);

    std::cout << "test_epg_controller passed" << std::endl;

    return 0;
}
