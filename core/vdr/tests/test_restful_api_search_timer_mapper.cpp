#include "RestfulApiSearchTimerMapper.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    const std::string json =
        "{\"searchtimers\":["
        "{\"id\":1,\"search\":\"Terra X\",\"use_as_searchtimer\":1},"
        "{\"id\":2,\"search\":\"Tatort\",\"use_as_searchtimer\":0}"
        "],\"count\":2,\"total\":2}";

    std::vector<SearchTimer> timers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            "livingroom",
            json);

    assert(timers.size() == 2);
    assert(timers.at(0).backendId() == "livingroom");
    assert(timers.at(0).backendNativeId() == "1");
    assert(timers.at(0).name() == "Terra X");
    assert(timers.at(0).query() == "Terra X");
    assert(timers.at(0).isActive());

    assert(timers.at(1).backendId() == "livingroom");
    assert(timers.at(1).backendNativeId() == "2");
    assert(timers.at(1).name() == "Tatort");
    assert(timers.at(1).query() == "Tatort");
    assert(timers.at(1).isInactive());

    const std::string escapedJson =
        "{\"searchtimers\":["
        "{\"id\":3,\"search\":\"Bob \\\"Marley\\\"\",\"use_as_searchtimer\":1}"
        "]}";

    std::vector<SearchTimer> escapedTimers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            "livingroom",
            escapedJson);

    assert(escapedTimers.size() == 1);
    assert(escapedTimers.at(0).name() == "Bob \"Marley\"");

    std::vector<SearchTimer> emptyTimers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            "livingroom",
            "{\"searchtimers\":[],\"count\":0,\"total\":0}");

    assert(emptyTimers.empty());

    std::vector<SearchTimer> invalidTimers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            "",
            json);

    assert(invalidTimers.empty());

    std::cout << "test_restful_api_search_timer_mapper passed" << std::endl;
    return 0;
}