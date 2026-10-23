#include "RestfulApiTimerMapper.h"
#include "VdrTimer.h"

#include <cassert>
#include <string>
#include <vector>

static void test_parse_timers_empty_array()
{
    const std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers("{\"timers\":[]}");

    assert(timers.empty());
}

static void test_parse_timers_maps_real_restfulapi_shape()
{
    const std::string json =
        "{\"timers\":["
        "{\"id\":\"12\","
        "\"index\":1,"
        "\"flags\":5,"
        "\"start\":2015,"
        "\"start_timestamp\":\"1780000000\","
        "\"stop_timestamp\":\"1780000900\","
        "\"stop\":2030,"
        "\"priority\":50,"
        "\"lifetime\":99,"
        "\"event_id\":56748,"
        "\"weekdays\":\"-------\","
        "\"day\":\"2026-05-28\","
        "\"channel\":\"C-61441-10006-50021\","
        "\"filename\":\"News~Tagesschau\","
        "\"channel_name\":\"Das Erste HD\","
        "\"is_pending\":true,"
        "\"is_recording\":false,"
        "\"is_active\":true,"
        "\"aux\":\"20 Uhr\""
        "},"
        "{\"id\":\"13\","
        "\"index\":2,"
        "\"flags\":8,"
        "\"start\":2115,"
        "\"stop\":2245,"
        "\"priority\":60,"
        "\"lifetime\":30,"
        "\"event_id\":56749,"
        "\"weekdays\":\"MTWTF--\","
        "\"day\":\"\","
        "\"channel\":\"C-61441-10000-52876\","
        "\"filename\":\"Tatort\","
        "\"channel_name\":\"NDR HD\","
        "\"aux\":\"Borowski und das Meer\""
        "}"
        "]}";

    const std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers(json);

    assert(timers.size() == 2);

    assert(timers[0].id == "12");
    assert(timers[0].channelId == "C-61441-10006-50021");
    assert(timers[0].channelName == "Das Erste HD");
    assert(timers[0].eventId == "56748");
    assert(timers[0].title == "Tagesschau");
    assert(timers[0].directory == "News");
    assert(timers[0].subtitle == "20 Uhr");
    assert(timers[0].aux == "20 Uhr");
    assert(timers[0].day == "2026-05-28");
    assert(timers[0].weekdays == "-------");
    assert(timers[0].startTime == "2015");
    assert(timers[0].endTime == "2030");
    assert(timers[0].flags == 5);
    assert(timers[0].priority == 50);
    assert(timers[0].lifetime == 99);
    assert(timers[0].enabled);
    assert(timers[0].vps);
    assert(!timers[0].recording);
    assert(timers[0].pending);

    assert(timers[1].id == "13");
    assert(timers[1].title == "Tatort");
    assert(timers[1].directory.empty());
    assert(timers[1].day.empty());
    assert(timers[1].weekdays == "MTWTF--");
    assert(timers[1].flags == 8);
    assert(!timers[1].enabled);
    assert(!timers[1].vps);
    assert(timers[1].recording);
    assert(!timers[1].pending);
}

static void test_parse_timers_falls_back_to_number_and_flags()
{
    const std::string json =
        "{\"timers\":["
        "{\"number\":7,"
        "\"flags\":5,"
        "\"channel_id\":\"C-1-1107-898\","
        "\"title\":\"Fallback Timer\","
        "\"start_time\":1780000000,"
        "\"stop_time\":1780000900"
        "}"
        "]}";

    const std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers(json);

    assert(timers.size() == 1);
    assert(timers[0].id == "7");
    assert(timers[0].title == "Fallback Timer");
    assert(timers[0].weekdays == "-------");
    assert(timers[0].enabled);
    assert(timers[0].vps);
}

static void test_parse_timers_does_not_invent_event_from_null()
{
    // Live RESTfulAPI may expose event_id:null before VDR associates EPG.
    // The former string parser consumed the next key ("weekdays") as the
    // event ID, persisting a bogus observed-state fingerprint.
    const auto timers = RestfulApiTimerMapper::parseTimers(
        "{\"timers\":[{\"id\":\"native:1\",\"channel\":\"C-1-1051-10301\","
        "\"filename\":\"Tagesschau\",\"event_id\":null,"
        "\"weekdays\":\"-------\",\"day\":\"2026-10-21\","
        "\"start\":1400,\"stop\":1410,\"flags\":1,"
        "\"priority\":50,\"lifetime\":99}]}");
    assert(timers.size() == 1);
    assert(timers[0].eventId.empty());
    assert(timers[0].weekdays == "-------");

    const auto numeric = RestfulApiTimerMapper::parseTimers(
        "{\"timers\":[{\"id\":\"native:2\",\"event_id\":13546,"
        "\"weekdays\":\"-------\"}]}");
    assert(numeric.size() == 1);
    assert(numeric[0].eventId == "13546");

    const auto quoted = RestfulApiTimerMapper::parseTimers(
        "{\"timers\":[{\"id\":\"native:3\",\"event_id\":\"13546\","
        "\"weekdays\":\"-------\"}]}");
    assert(quoted.size() == 1);
    assert(quoted[0].eventId == "13546");
}

static void test_parse_timers_ignores_objects_without_id()
{
    const std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers(
            "{\"timers\":[{\"title\":\"Broken Timer\"}]}");

    assert(timers.empty());
}

static void test_parse_timers_tolerates_invalid_json()
{
    const std::vector<VdrTimer> timers =
        RestfulApiTimerMapper::parseTimers("not json");

    assert(timers.empty());
}

int main()
{
    test_parse_timers_empty_array();
    test_parse_timers_maps_real_restfulapi_shape();
    test_parse_timers_falls_back_to_number_and_flags();
    test_parse_timers_does_not_invent_event_from_null();
    test_parse_timers_ignores_objects_without_id();
    test_parse_timers_tolerates_invalid_json();
    return 0;
}
