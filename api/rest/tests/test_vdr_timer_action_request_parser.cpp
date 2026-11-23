#include "VdrTimerActionRequestParser.h"

#include <cassert>

static void test_parse_full_timer_action_request()
{
    VdrTimerActionRequestParser parser;

    const VdrTimerOperationRequest request =
        parser.parse(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\","
            "\"channelId\":\"C-61441-10006-50021\","
            "\"title\":\"Tagesschau\","
            "\"directory\":\"News\","
            "\"day\":\"2026-06-18\","
            "\"weekdays\":\"-------\","
            "\"start\":2015,"
            "\"stop\":2030,"
            "\"priority\":50,"
            "\"lifetime\":99,"
            "\"active\":true,"
            "\"vps\":true,"
            "\"aux\":\"<epgsearch></epgsearch>\""
            "}");

    assert(request.backendId == "living-room");
    assert(request.timerId == "42");
    assert(request.channelId == "C-61441-10006-50021");
    assert(request.title == "Tagesschau");
    assert(request.directory == "News");
    assert(request.day == "2026-06-18");
    assert(request.weekdays == "-------");
    assert(request.start == 2015);
    assert(request.stop == 2030);
    assert(request.priority == 50);
    assert(request.lifetime == 99);
    assert(request.active);
    assert(request.vps);
    assert(request.aux == "<epgsearch></epgsearch>");
}

static void test_parse_minimal_request_uses_defaults()
{
    VdrTimerActionRequestParser parser;

    const VdrTimerOperationRequest request =
        parser.parse(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\""
            "}");

    assert(request.backendId == "living-room");
    assert(request.timerId == "42");
    assert(request.weekdays == "-------");
    assert(request.priority == 50);
    assert(request.lifetime == 99);
    assert(request.active);
    assert(!request.vps);
    assert(request.start == 0);
    assert(request.stop == 0);
}

static void test_parse_false_flags()
{
    VdrTimerActionRequestParser parser;

    const VdrTimerOperationRequest request =
        parser.parse("{\"active\":false,\"vps\":false}");

    assert(!request.active);
    assert(!request.vps);
}

static void test_parse_numeric_flags()
{
    VdrTimerActionRequestParser parser;

    const VdrTimerOperationRequest request =
        parser.parse("{\"active\":1,\"vps\":1}");

    assert(request.active);
    assert(request.vps);
}

static void test_parse_metadata_with_commas_braces_and_json_escapes()
{
    VdrTimerActionRequestParser parser;
    const VdrTimerOperationRequest request = parser.parse(
        R"json({"backendId":"living-room","timerId":"42","title":"Serie, Teil } 2: \"Finale\"","aux":"<vdr a=\"A,B}\" b=\"c\\d\">Name\nZeile 2</vdr>","start":2015,"stop":2030,"priority":0,"lifetime":0,"vps":true})json");
    assert(request.backendId == "living-room");
    assert(request.timerId == "42");
    assert(request.title == "Serie, Teil } 2: \"Finale\"");
    assert(request.aux == "<vdr a=\"A,B}\" b=\"c\\d\">Name\nZeile 2</vdr>");
    assert(request.start == 2015);
    assert(request.stop == 2030);
    assert(request.priority == 0);
    assert(request.lifetime == 0);
    assert(request.vps);
}

static void test_parse_unicode_metadata()
{
    VdrTimerActionRequestParser parser;
    const VdrTimerOperationRequest request = parser.parse(
        R"json({"title":"K\u00f6ln \ud83c\udfa5","aux":"<data>foo=bar&amp;baz</data>"})json");
    assert(request.title == "K\xC3\xB6ln \xF0\x9F\x8E\xA5");
    assert(request.aux == "<data>foo=bar&amp;baz</data>");
}

int main()
{
    test_parse_full_timer_action_request();
    test_parse_minimal_request_uses_defaults();
    test_parse_false_flags();
    test_parse_numeric_flags();
    test_parse_metadata_with_commas_braces_and_json_escapes();
    test_parse_unicode_metadata();
    return 0;
}
