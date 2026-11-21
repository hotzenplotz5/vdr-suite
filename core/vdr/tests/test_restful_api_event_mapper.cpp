#include "RestfulApiEventMapper.h"
#include "VdrEvent.h"

#include <cassert>
#include <string>
#include <vector>

static void test_parse_events_empty_array()
{
    std::vector<VdrEvent> events =
        RestfulApiEventMapper::parseEvents("{\"events\":[]}");

    assert(events.empty() == true);
}

static void test_parse_events_maps_real_restfulapi_shape()
{
    const std::string json =
        "{\"events\":["
        "{\"id\":56748,"
        "\"title\":\"kinokino\","
        "\"short_text\":\"Filmmagazin\","
        "\"description\":\"Filmmagazin Beschreibung\","
        "\"start_time\":1780453800,"
        "\"channel\":\"C-61441-10023-10376\","
        "\"duration\":900,"
        "\"parental_rating\":0,"
        "\"contents\":[]"
        "},"
        "{\"id\":56761,"
        "\"title\":\"Zero\","
        "\"short_text\":\"Fernsehfilm Deutschland 2021\","
        "\"description\":\"Berlin in naher Zukunft\","
        "\"start_time\":1780487700,"
        "\"channel\":\"C-61441-10023-10376\","
        "\"duration\":5400,"
        "\"parental_rating\":6,"
        "\"contents\":[\"Film/Drama\",\"Detektiv/Thriller\"]"
        "}"
        "]}";

    std::vector<VdrEvent> events =
        RestfulApiEventMapper::parseEvents(json);

    assert(events.size() == 2);

    assert(events[0].id == "56748");
    assert(events[0].channelId == "C-61441-10023-10376");
    assert(events[0].title == "kinokino");
    assert(events[0].subtitle == "Filmmagazin");
    assert(events[0].description == "Filmmagazin Beschreibung");
    assert(events[0].startTime == "1780453800");
    assert(events[0].endTime == "1780454700");
    assert(events[0].durationSeconds == 900);
    assert(events[0].contentDescriptors.empty() == true);
    assert(events[0].parentalRating == 0);

    assert(events[1].id == "56761");
    assert(events[1].channelId == "C-61441-10023-10376");
    assert(events[1].title == "Zero");
    assert(events[1].subtitle == "Fernsehfilm Deutschland 2021");
    assert(events[1].description == "Berlin in naher Zukunft");
    assert(events[1].startTime == "1780487700");
    assert(events[1].endTime == "1780493100");
    assert(events[1].durationSeconds == 5400);
    assert(events[1].contentDescriptors.size() == 2);
    assert(events[1].contentDescriptors[0] == "Film/Drama");
    assert(events[1].contentDescriptors[1] == "Detektiv/Thriller");
    assert(events[1].parentalRating == 6);
}

static void test_parse_events_decodes_unicode_escapes()
{
    const std::string json =
        "{\"events\":["
        "{\"id\":99,"
        "\"title\":\"Der gro\\u00dfe B\\u00f6rsencrash\","
        "\"short_text\":\"M\\u00fcnchen\","
        "\"description\":\"Doku f\\u00fcr Kinder\","
        "\"start_time\":1780453800,"
        "\"channel\":\"C-1-2-3\","
        "\"duration\":900,"
        "\"contents\":[\"Doku/\\u00d6konomie\"]"
        "}"
        "]}";

    std::vector<VdrEvent> events =
        RestfulApiEventMapper::parseEvents(json);

    assert(events.size() == 1);
    assert(events[0].title == "Der große Börsencrash");
    assert(events[0].subtitle == "München");
    assert(events[0].description == "Doku für Kinder");
    assert(events[0].contentDescriptors.size() == 1);
    assert(events[0].contentDescriptors[0] == "Doku/Ökonomie");
}

static void test_parse_events_accepts_top_level_array()
{
    const std::string json =
        "[{\"id\":42,"
        "\"title\":\"Top Level Event\","
        "\"short_text\":\"Subtitle\","
        "\"description\":\"Description\","
        "\"start_time\":1780000000,"
        "\"channel\":\"C-1-2-3\","
        "\"duration\":600,"
        "\"parental_rating\":12,"
        "\"contents\":[\"News\"]"
        "}]";

    std::vector<VdrEvent> events =
        RestfulApiEventMapper::parseEvents(json);

    assert(events.size() == 1);
    assert(events[0].id == "42");
    assert(events[0].channelId == "C-1-2-3");
    assert(events[0].title == "Top Level Event");
    assert(events[0].subtitle == "Subtitle");
    assert(events[0].description == "Description");
    assert(events[0].startTime == "1780000000");
    assert(events[0].endTime == "1780000600");
    assert(events[0].durationSeconds == 600);
    assert(events[0].contentDescriptors.size() == 1);
    assert(events[0].contentDescriptors[0] == "News");
    assert(events[0].parentalRating == 12);
}

static void test_parse_events_ignores_objects_without_id()
{
    const std::string json =
        "{\"events\":["
        "{\"title\":\"Broken Event\","
        "\"start_time\":1780000000,"
        "\"channel\":\"C-1-2-3\","
        "\"duration\":600"
        "}"
        "]}";

    std::vector<VdrEvent> events =
        RestfulApiEventMapper::parseEvents(json);

    assert(events.empty() == true);
}

static void test_parse_events_tolerates_invalid_json()
{
    std::vector<VdrEvent> events =
        RestfulApiEventMapper::parseEvents("not json");

    assert(events.empty() == true);
}

int main()
{
    test_parse_events_empty_array();
    test_parse_events_maps_real_restfulapi_shape();
    test_parse_events_decodes_unicode_escapes();
    test_parse_events_accepts_top_level_array();
    test_parse_events_ignores_objects_without_id();
    test_parse_events_tolerates_invalid_json();

    return 0;
}
