#include "SearchTimerCreateResultJsonSerializer.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerCreateResult result =
        SearchTimerCreateResult::ok(
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    "home-vdr",
                    "created-1"),
                "Terra X Suche",
                "Terra X",
                SearchTimerState::Active),
            "searchtimer created");

    SearchTimerCreateResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"success\":true") != std::string::npos);
    assert(json.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"created-1\"") != std::string::npos);
    assert(json.find("\"name\":\"Terra X Suche\"") != std::string::npos);
    assert(json.find("\"query\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"state\":\"active\"") != std::string::npos);
    assert(json.find("\"message\":\"searchtimer created\"") != std::string::npos);
    assert(json.find("\"warnings\":[]") != std::string::npos);
    assert(json.find("\"errors\":[]") != std::string::npos);

    std::cout
        << "test_search_timer_create_result_json_serializer passed"
        << std::endl;

    return 0;
}
