#include "RestfulApiTimerMapper.h"
#include "VdrTimer.h"

#include <cassert>
#include <string>
#include <vector>

static void test_parse_timers_empty_array()
{
    std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers("{\"timers\":[]}");

    assert(timers.empty() == true);
}

static void test_parse_timers_maps_real_restfulapi_shape()
{
    const std::string json =
        "{\"timers\":["
        "{\"id\":\"12\","
        "\"channel_id\":\"C-61441-10006-50021\","
        "\"event_id\":\"56748\","
        "\"title\":\"Tagesschau\","
        "\"short_text\":\"20 Uhr\","
        "\"start_time\":1780000000,"
        "\"stop_time\":1780000900,"
        "\"priority\":50,"
        "\"lifetime\":99,"
        "\"active\":true,"
        "\"recording\":false"
        "},"
        "{\"id\":\"13\","
        "\"channelid\":\"C-61441-10000-52876\","
        "\"eventid\":\"56749\","
        "\"file\":\"Tatort\","
        "\"aux\":\"Borowski und das Meer\","
        "\"starttime\":1780001000,"
        "\"stoptime\":1780006400,"
        "\"priority\":60,"
        "\"lifetime\":30,"
        "\"active\":false,"
        "\"recording\":true"
        "}"
        "]}";

    std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers(json);

    assert(timers.size() == 2);

    assert(timers[0].id == "12");
    assert(timers[0].channelId == "C-61441-10006-50021");
    assert(timers[0].eventId == "56748");
    assert(timers[0].title == "Tagesschau");
    assert(timers[0].subtitle == "20 Uhr");
    assert(timers[0].startTime == "1780000000");
    assert(timers[0].endTime == "1780000900");
    assert(timers[0].priority == 50);
    assert(timers[0].lifetime == 99);
    assert(timers[0].enabled == true);
    assert(timers[0].recording == false);

    assert(timers[1].id == "13");
    assert(timers[1].channelId == "C-61441-10000-52876");
    assert(timers[1].eventId == "56749");
    assert(timers[1].title == "Tatort");
    assert(timers[1].subtitle == "Borowski und das Meer");
    assert(timers[1].startTime == "1780001000");
    assert(timers[1].endTime == "1780006400");
    assert(timers[1].priority == 60);
    assert(timers[1].lifetime == 30);
    assert(timers[1].enabled == false);
    assert(timers[1].recording == true);
}

static void test_parse_timers_falls_back_to_number_as_id()
{
    const std::string json =
        "{\"timers\":["
        "{\"number\":7,"
        "\"channel_id\":\"C-1-1107-898\","
        "\"title\":\"Fallback Timer\","
        "\"start_time\":1780000000,"
        "\"stop_time\":1780000900"
        "}"
        "]}";

    std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers(json);

    assert(timers.size() == 1);
    assert(timers[0].id == "7");
    assert(timers[0].title == "Fallback Timer");
}

static void test_parse_timers_ignores_objects_without_id()
{
    const std::string json =
        "{\"timers\":["
        "{\"title\":\"Broken Timer\","
        "\"start_time\":1780000000,"
        "\"stop_time\":1780000900"
        "}"
        "]}";

    std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers(json);

    assert(timers.empty() == true);
}

static void test_parse_timers_tolerates_invalid_json()
{
    std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers("not json");

    assert(timers.empty() == true);
}

int main()
{
    test_parse_timers_empty_array();
    test_parse_timers_maps_real_restfulapi_shape();
    test_parse_timers_falls_back_to_number_as_id();
    test_parse_timers_ignores_objects_without_id();
    test_parse_timers_tolerates_invalid_json();

    return 0;
}
