#include "RestfulApiSearchTimerMapper.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    const std::string json =
        "{\"searchtimers\":["
        "{\"id\":1,\"search\":\"Terra X\",\"use_as_searchtimer\":1,"
        "\"directory\":\"Doku\",\"priority\":50,\"lifetime\":99,"
        "\"margin_start\":5,\"margin_stop\":10,\"use_vps\":1,"
        "\"use_channel\":1,\"use_dayofweek\":1,\"use_duration\":1,"
        "\"duration_min\":30,\"duration_max\":120,"
        "\"compare_title\":1,\"compare_subtitle\":1,\"compare_summary\":1,"
        "\"compare_categories\":1,\"compare_time\":1,"
        "\"avoid_repeats\":1,\"allowed_repeats\":3,\"repeats_within_days\":14},"
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
    assert(timers.at(0).recordingOptions().directory() == "Doku");
    assert(timers.at(0).recordingOptions().priority() == 50);
    assert(timers.at(0).recordingOptions().lifetime() == 99);
    assert(timers.at(0).scheduleOptions().marginStartMinutes() == 5);
    assert(timers.at(0).scheduleOptions().marginStopMinutes() == 10);
    assert(timers.at(0).scheduleOptions().useVps());
    assert(timers.at(0).filterOptions().useChannel());
    assert(timers.at(0).filterOptions().useDayOfWeek());
    assert(timers.at(0).filterOptions().useDuration());
    assert(timers.at(0).filterOptions().durationMinMinutes() == 30);
    assert(timers.at(0).filterOptions().durationMaxMinutes() == 120);
    assert(timers.at(0).comparisonOptions().compareTitle());
    assert(timers.at(0).comparisonOptions().compareSubtitle());
    assert(timers.at(0).comparisonOptions().compareSummary());
    assert(timers.at(0).comparisonOptions().compareCategories());
    assert(timers.at(0).comparisonOptions().compareTime());
    assert(timers.at(0).repeatOptions().avoidRepeats());
    assert(timers.at(0).repeatOptions().allowedRepeats() == 3);
    assert(timers.at(0).repeatOptions().repeatsWithinDays() == 14);

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