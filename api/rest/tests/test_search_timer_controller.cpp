#include "SearchTimerController.h"

#include "SearchTimerResult.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerService.h"
#include "VdrEvent.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    SearchTimerService searchTimerService;
    SearchTimerResultJsonSerializer serializer;
    SearchTimerController controller(searchTimerService, serializer);

    std::vector<SearchTimer> timers;
    timers.push_back(SearchTimer::create(
        SearchTimerId::fromBackendNativeId("livingroom", "1"),
        "Terra X",
        "Terra X",
        SearchTimerState::Active));

    SearchTimerResult result = SearchTimerResult::from(
        timers,
        1,
        25,
        0);

    ApiResponse response = controller.getSearchTimers(result);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"searchtimers\"") != std::string::npos);
    assert(response.body.find("\"backendId\":\"livingroom\"") != std::string::npos);
    assert(response.body.find("\"backendNativeId\":\"1\"") != std::string::npos);
    assert(response.body.find("\"name\":\"Terra X\"") != std::string::npos);
    assert(response.body.find("\"state\":\"active\"") != std::string::npos);

    VdrEvent event;
    event.id = "event-1";
    event.channelId = "channel-1";
    event.title = "Terra X";
    event.startTime = "1000";
    event.endTime = "1100";
    event.durationSeconds = 3600;

    ApiResponse previewResponse =
        controller.previewSearchTimer(
            timers[0],
            {event},
            10,
            0);

    assert(previewResponse.statusCode == 200);
    assert(previewResponse.contentType == "application/json");
    assert(previewResponse.body.find("\"searchTimer\":{") != std::string::npos);
    assert(previewResponse.body.find("\"preview\":{") != std::string::npos);
    assert(previewResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(previewResponse.body.find("\"eventId\":\"event-1\"") != std::string::npos);

    std::cout << "test_search_timer_controller passed" << std::endl;
    return 0;
}