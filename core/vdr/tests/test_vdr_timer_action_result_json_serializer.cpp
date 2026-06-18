#include "VdrTimerActionResultJsonSerializer.h"

#include <cassert>

static void test_serialize_success_result()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::ok(
            VdrTimerActionType::Create,
            "timer-42",
            "living-room",
            "Timer created");

    result.warnings.push_back("dry run");

    VdrTimerActionResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"success\":true") != std::string::npos);
    assert(json.find("\"type\":\"create\"") != std::string::npos);
    assert(json.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(json.find("\"timerId\":\"timer-42\"") != std::string::npos);
    assert(json.find("\"message\":\"Timer created\"") != std::string::npos);
    assert(json.find("\"warnings\":[\"dry run\"]") != std::string::npos);
    assert(json.find("\"errors\":[]") != std::string::npos);
}

static void test_serialize_failure_result()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::failed(
            VdrTimerActionType::Delete,
            "timer-42",
            "living-room",
            "Timer delete failed",
            {"RESTfulAPI returned HTTP status 404: Timer id invalid!"});

    VdrTimerActionResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"success\":false") != std::string::npos);
    assert(json.find("\"type\":\"delete\"") != std::string::npos);
    assert(json.find("\"message\":\"Timer delete failed\"") != std::string::npos);
    assert(json.find("\"errors\":[\"RESTfulAPI returned HTTP status 404: Timer id invalid!\"]") != std::string::npos);
}

static void test_serialize_escapes_strings()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::failed(
            VdrTimerActionType::Update,
            "timer\"42",
            "living\\room",
            "Timer \"quoted\" failed",
            {"path contains \\"});

    VdrTimerActionResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("timer\\\"42") != std::string::npos);
    assert(json.find("living\\\\room") != std::string::npos);
    assert(json.find("Timer \\\"quoted\\\" failed") != std::string::npos);
    assert(json.find("path contains \\\\") != std::string::npos);
}

static void test_serialize_unknown_type()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::ok(
            VdrTimerActionType::Unknown,
            "timer-42",
            "living-room",
            "Timer action executed");

    VdrTimerActionResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"type\":\"unknown\"") != std::string::npos);
}


static void test_error_body_is_serialized_for_frontend_message()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::failed(
            VdrTimerActionType::Create,
            "42",
            "living-room",
            "RESTfulAPI timer action request failed",
            {"RESTfulAPI returned HTTP status 409: timer conflict: overlaps with existing timer 123"});

    VdrTimerActionResultJsonSerializer serializer;

    const std::string json = serializer.serialize(result);

    assert(json.find("\"success\":false") != std::string::npos);
    assert(json.find("\"type\":\"create\"") != std::string::npos);
    assert(json.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(json.find("\"timerId\":\"42\"") != std::string::npos);
    assert(json.find("timer conflict: overlaps with existing timer 123") != std::string::npos);
}

int main()
{
    test_error_body_is_serialized_for_frontend_message();
    test_serialize_success_result();
    test_serialize_failure_result();
    test_serialize_escapes_strings();
    test_serialize_unknown_type();

    return 0;
}
