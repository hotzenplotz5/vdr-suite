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

    ApiResponse timeWindowResponse = controller.getTimeWindow();
    assertEventResponse(timeWindowResponse, "event-time");
    assert(epgQueryService.lastCall == 2);
    assert(epgQueryService.lastTimespan == 7200);

    ApiResponse channelWindowResponse = controller.getChannelWindow();
    assertEventResponse(channelWindowResponse, "event-channel");
    assert(epgQueryService.lastCall == 3);
    assert(epgQueryService.lastLimit == 5);

    std::cout << "test_epg_controller passed" << std::endl;

    return 0;
}
