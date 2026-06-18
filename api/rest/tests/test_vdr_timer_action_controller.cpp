#include "VdrTimerActionController.h"

#include "MockVdrTimerActionExecutor.h"
#include "VdrTimerActionRequestParser.h"
#include "VdrTimerActionResultJsonSerializer.h"
#include "VdrTimerActionService.h"

#include <cassert>
#include <string>

static VdrTimerOperationRequest makeRequest()
{
    VdrTimerOperationRequest request;
    request.timerId = "42";
    request.backendId = "living-room";
    request.channelId = "C-61441-10006-50021";
    request.title = "Tagesschau";
    request.directory = "News";
    request.day = "2026-06-18";
    request.weekdays = "-------";
    request.start = 2015;
    request.stop = 2030;
    request.priority = 50;
    request.lifetime = 99;
    request.active = true;
    return request;
}

static void test_create_request_returns_json_response()
{
    VdrTimerActionService service;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionController controller(service, serializer);
    MockVdrTimerActionExecutor executor;

    const ApiResponse response =
        controller.create(makeRequest(), executor);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"success\":true") != std::string::npos);
    assert(response.body.find("\"type\":\"create\"") != std::string::npos);
    assert(response.body.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(response.body.find("\"timerId\":\"42\"") != std::string::npos);
}

static void test_update_request_returns_json_response()
{
    VdrTimerActionService service;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionController controller(service, serializer);
    MockVdrTimerActionExecutor executor;

    const ApiResponse response =
        controller.update(makeRequest(), executor);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"type\":\"update\"") != std::string::npos);
}

static void test_remove_request_returns_json_response()
{
    VdrTimerActionService service;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionController controller(service, serializer);
    MockVdrTimerActionExecutor executor;

    const ApiResponse response =
        controller.remove(makeRequest(), executor);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"type\":\"delete\"") != std::string::npos);
}

static void test_create_body_uses_parser()
{
    VdrTimerActionService service;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;
    VdrTimerActionController controller(service, serializer, parser);
    MockVdrTimerActionExecutor executor;

    const ApiResponse response =
        controller.createBody(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\","
            "\"channelId\":\"C-61441-10006-50021\","
            "\"title\":\"Tagesschau\""
            "}",
            executor);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"type\":\"create\"") != std::string::npos);
    assert(response.body.find("\"timerId\":\"42\"") != std::string::npos);
}

static void test_update_body_uses_parser()
{
    VdrTimerActionService service;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;
    VdrTimerActionController controller(service, serializer, parser);
    MockVdrTimerActionExecutor executor;

    const ApiResponse response =
        controller.updateBody(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\""
            "}",
            executor);

    assert(response.statusCode == 200);
    assert(response.body.find("\"type\":\"update\"") != std::string::npos);
}

static void test_remove_body_uses_parser()
{
    VdrTimerActionService service;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;
    VdrTimerActionController controller(service, serializer, parser);
    MockVdrTimerActionExecutor executor;

    const ApiResponse response =
        controller.removeBody(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\""
            "}",
            executor);

    assert(response.statusCode == 200);
    assert(response.body.find("\"type\":\"delete\"") != std::string::npos);
}

static void test_body_request_without_parser_returns_500()
{
    VdrTimerActionService service;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionController controller(service, serializer);
    MockVdrTimerActionExecutor executor;

    const ApiResponse response =
        controller.createBody("{}", executor);

    assert(response.statusCode == 500);
    assert(response.contentType == "application/json");
    assert(response.body == "{\"error\":\"vdr timer action request parser unavailable\"}");
}

int main()
{
    test_create_request_returns_json_response();
    test_update_request_returns_json_response();
    test_remove_request_returns_json_response();
    test_create_body_uses_parser();
    test_update_body_uses_parser();
    test_remove_body_uses_parser();
    test_body_request_without_parser_returns_500();

    return 0;
}
