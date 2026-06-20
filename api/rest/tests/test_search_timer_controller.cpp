#include "SearchTimerController.h"

#include "SearchTimerResult.h"
#include "SearchTimerResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    SearchTimerResultJsonSerializer serializer;
    SearchTimerController controller(serializer);

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

    std::cout << "test_search_timer_controller passed" << std::endl;
    return 0;
}