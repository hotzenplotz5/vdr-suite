#include "EpgController.h"
#include "EpgSearchResultJsonSerializer.h"
#include "EpgSearchService.h"
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
        return {
            makeEvent("event-time", "channel-1", "Tagesschau"),
            makeEvent("event-search", "channel-1", "Tatort")
        };
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
    static VdrEvent makeEvent(
        const std::string& id,
        const std::string& channelId,
        const std::string& title)
    {
        VdrEvent event;
        event.id = id;
        event.channelId = channelId;
        event.title = title;
        event.subtitle = "Subtitle";
        event.description = "Description";
        event.startTime = "1780000000";
        event.endTime = "1780003600";
        event.durationSeconds = 3600;

        return event;
    }

    static std::vector<VdrEvent> makeEvents(const std::string& id)
    {
        return {makeEvent(id, "channel-1", "Title")};
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
    EpgSearchService epgSearchService;
    EpgSearchResultJsonSerializer epgSearchResultJsonSerializer;

    EpgController controller(
        epgQueryService,
        epgSearchService,
        epgSearchResultJsonSerializer);

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

    ApiResponse caseInsensitiveSearchResponse =
        controller.search(
            "TATORT",
            "living-room",
            "channel-1",
            1780000000,
            7200,
            10,
            0,
            "title",
            "asc");

    assert(caseInsensitiveSearchResponse.statusCode == 200);
    assert(caseInsensitiveSearchResponse.contentType == "application/json");
    assert(caseInsensitiveSearchResponse.body.find("\"matches\":[") != std::string::npos);
    assert(caseInsensitiveSearchResponse.body.find("\"id\":\"event-search\"") != std::string::npos);
    assert(caseInsensitiveSearchResponse.body.find("event-time") == std::string::npos);

    ApiResponse exactModeSearchResponse =
        controller.search(
            "Tatort",
            "living-room",
            "channel-1",
            1780000000,
            7200,
            10,
            0,
            "title",
            "asc",
            "exact");

    assert(exactModeSearchResponse.statusCode == 200);
    assert(exactModeSearchResponse.contentType == "application/json");
    assert(exactModeSearchResponse.body.find("\"id\":\"event-search\"") != std::string::npos);
    assert(exactModeSearchResponse.body.find("event-time") == std::string::npos);

    ApiResponse exactModeNoMatchResponse =
        controller.search(
            "Tat",
            "living-room",
            "channel-1",
            1780000000,
            7200,
            10,
            0,
            "title",
            "asc",
            "exact");

    assert(exactModeNoMatchResponse.statusCode == 200);
    assert(exactModeNoMatchResponse.contentType == "application/json");
    assert(exactModeNoMatchResponse.body.find("\"id\":\"event-search\"") == std::string::npos);

    ApiResponse allWordsModeSearchResponse =
        controller.search(
            "Tatort Subtitle",
            "living-room",
            "channel-1",
            1780000000,
            7200,
            10,
            0,
            "title",
            "asc",
            "all");

    assert(allWordsModeSearchResponse.statusCode == 200);
    assert(allWordsModeSearchResponse.contentType == "application/json");
    assert(allWordsModeSearchResponse.body.find("\"id\":\"event-search\"") != std::string::npos);
    assert(allWordsModeSearchResponse.body.find("event-time") == std::string::npos);

    ApiResponse anyWordModeSearchResponse =
        controller.search(
            "Tatort Tagesschau",
            "living-room",
            "channel-1",
            1780000000,
            7200,
            10,
            0,
            "title",
            "asc",
            "any");

    assert(anyWordModeSearchResponse.statusCode == 200);
    assert(anyWordModeSearchResponse.contentType == "application/json");
    assert(anyWordModeSearchResponse.body.find("\"id\":\"event-search\"") != std::string::npos);
    assert(anyWordModeSearchResponse.body.find("\"id\":\"event-time\"") != std::string::npos);

    ApiResponse invalidModeResponse =
        controller.search(
            "tatort",
            "",
            "",
            -1,
            7200,
            10,
            0,
            "",
            "",
            "sideways");

    assert(invalidModeResponse.statusCode == 400);
    assert(invalidModeResponse.body.find("invalid search mode") != std::string::npos);

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

    ApiResponse searchResponse =
        controller.search(
            "tatort",
            "living-room",
            "channel-1",
            1780000000,
            7200,
            10,
            0,
            "title",
            "asc");

    assert(searchResponse.statusCode == 200);
    assert(searchResponse.contentType == "application/json");
    assert(searchResponse.body.find("\"matches\":[") != std::string::npos);
    assert(searchResponse.body.find("\"id\":\"event-search\"") != std::string::npos);
    assert(searchResponse.body.find("\"matchedFields\":[]") != std::string::npos);
    assert(searchResponse.body.find("event-time") == std::string::npos);
    assert(epgQueryService.lastCall == 2);
    assert(epgQueryService.lastChannelId == "channel-1");
    assert(epgQueryService.lastFrom == 1780000000);
    assert(epgQueryService.lastTimespan == 7200);

    ApiResponse invalidTimespanResponse =
        controller.search(
            "tatort",
            "",
            "",
            -1,
            0,
            10,
            0,
            "",
            "");

    assert(invalidTimespanResponse.statusCode == 400);
    assert(invalidTimespanResponse.contentType == "application/json");
    assert(invalidTimespanResponse.body.find("timespan") != std::string::npos);

    ApiResponse invalidLimitResponse =
        controller.search(
            "tatort",
            "",
            "",
            -1,
            7200,
            -1,
            0,
            "",
            "");

    assert(invalidLimitResponse.statusCode == 400);
    assert(invalidLimitResponse.body.find("limit") != std::string::npos);

    ApiResponse invalidOffsetResponse =
        controller.search(
            "tatort",
            "",
            "",
            -1,
            7200,
            10,
            -1,
            "",
            "");

    assert(invalidOffsetResponse.statusCode == 400);
    assert(invalidOffsetResponse.body.find("offset") != std::string::npos);

    ApiResponse invalidSortResponse =
        controller.search(
            "tatort",
            "",
            "",
            -1,
            7200,
            10,
            0,
            "invalid",
            "");

    assert(invalidSortResponse.statusCode == 400);
    assert(invalidSortResponse.body.find("sort") != std::string::npos);

    ApiResponse invalidOrderResponse =
        controller.search(
            "tatort",
            "",
            "",
            -1,
            7200,
            10,
            0,
            "",
            "sideways");

    assert(invalidOrderResponse.statusCode == 400);
    assert(invalidOrderResponse.body.find("order") != std::string::npos);

    std::cout << "test_epg_controller passed" << std::endl;

    return 0;
}
