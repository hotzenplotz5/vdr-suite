#include "SearchTimerDeleteResultJsonSerializer.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerDeleteResult result =
        SearchTimerDeleteResult::ok(
            "home-vdr",
            "searchtimer-42",
            "searchtimer deleted");

    SearchTimerDeleteResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"success\":true") != std::string::npos);
    assert(json.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"searchtimer-42\"") != std::string::npos);
    assert(json.find("\"message\":\"searchtimer deleted\"") != std::string::npos);
    assert(json.find("\"warnings\":[]") != std::string::npos);
    assert(json.find("\"errors\":[]") != std::string::npos);

    std::cout
        << "test_search_timer_delete_result_json_serializer passed"
        << std::endl;

    return 0;
}
