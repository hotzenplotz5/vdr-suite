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
    assert(request.active == true);
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
    assert(request.priority == 50);
    assert(request.lifetime == 99);
    assert(request.active == true);
    assert(request.start == 0);
    assert(request.stop == 0);
}

static void test_parse_false_active()
{
    VdrTimerActionRequestParser parser;

    const VdrTimerOperationRequest request =
        parser.parse(
            "{"
            "\"active\":false"
            "}");

    assert(request.active == false);
}

static void test_parse_numeric_active()
{
    VdrTimerActionRequestParser parser;

    const VdrTimerOperationRequest request =
        parser.parse(
            "{"
            "\"active\":1"
            "}");

    assert(request.active == true);
}

int main()
{
    test_parse_full_timer_action_request();
    test_parse_minimal_request_uses_defaults();
    test_parse_false_active();
    test_parse_numeric_active();

    return 0;
}
