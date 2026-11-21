#include "SearchTimerUpdateResultJsonSerializer.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerUpdateResult result =
        SearchTimerUpdateResult::ok(
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    "home-vdr",
                    "searchtimer-42"),
                "Terra X Suche",
                "Terra X",
                SearchTimerState::Inactive),
            "searchtimer updated");

    SearchTimerUpdateResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"success\":true") != std::string::npos);
    assert(json.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"searchtimer-42\"") != std::string::npos);
    assert(json.find("\"name\":\"Terra X Suche\"") != std::string::npos);
    assert(json.find("\"query\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"state\":\"inactive\"") != std::string::npos);
    assert(json.find("\"message\":\"searchtimer updated\"") != std::string::npos);
    assert(json.find("\"warnings\":[]") != std::string::npos);
    assert(json.find("\"errors\":[]") != std::string::npos);

    std::cout
        << "test_search_timer_update_result_json_serializer passed"
        << std::endl;

    return 0;
}
