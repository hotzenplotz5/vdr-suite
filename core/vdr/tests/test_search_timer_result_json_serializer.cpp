#include "SearchTimerResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    SearchTimerResultJsonSerializer serializer;

    SearchTimerResult emptyResult = SearchTimerResult::empty(25, 10);
    const std::string emptyJson = serializer.serialize(emptyResult);
    assert(emptyJson.find("\"totalCount\":0") != std::string::npos);
    assert(emptyJson.find("\"returnedCount\":0") != std::string::npos);
    assert(emptyJson.find("\"limit\":25") != std::string::npos);
    assert(emptyJson.find("\"offset\":10") != std::string::npos);
    assert(emptyJson.find("\"searchtimers\":[]") != std::string::npos);

    std::vector<SearchTimer> timers;
    timers.push_back(SearchTimer::create(
        SearchTimerId::fromBackendNativeId("livingroom", "1"),
        "Terra X",
        "Terra X",
        SearchTimerState::Active));
    timers.push_back(SearchTimer::create(
        SearchTimerId::fromBackendNativeId("bedroom", "2"),
        "Bob \"Marley\"",
        "Bob\\Marley",
        SearchTimerState::Inactive));

    SearchTimerResult result = SearchTimerResult::from(
        timers,
        7,
        2,
        4);

    const std::string json = serializer.serialize(result);

    assert(json.find("\"totalCount\":7") != std::string::npos);
    assert(json.find("\"returnedCount\":2") != std::string::npos);
    assert(json.find("\"limit\":2") != std::string::npos);
    assert(json.find("\"offset\":4") != std::string::npos);
    assert(json.find("\"backendId\":\"livingroom\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"1\"") != std::string::npos);
    assert(json.find("\"name\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"query\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"state\":\"active\"") != std::string::npos);
    assert(json.find("\"backendId\":\"bedroom\"") != std::string::npos);
    assert(json.find("\"name\":\"Bob \\\"Marley\\\"\"") != std::string::npos);
    assert(json.find("\"query\":\"Bob\\\\Marley\"") != std::string::npos);
    assert(json.find("\"state\":\"inactive\"") != std::string::npos);

    std::cout << "test_search_timer_result_json_serializer passed" << std::endl;
    return 0;
}