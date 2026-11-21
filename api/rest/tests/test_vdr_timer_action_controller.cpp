#include "VdrTimerActionController.h"

#include "BackendAccessPolicy.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "MockVdrTimerActionExecutor.h"
#include "VdrTimerActionExecutionService.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "VdrTimerActionRequestParser.h"
#include "VdrTimerActionResultJsonSerializer.h"
#include "VdrTimerActionService.h"

#include <cassert>
#include <memory>
#include <string>

class TestTimerActionExecutorAdapter final : public IVdrTimerActionExecutorAdapter
{
public:
    explicit TestTimerActionExecutorAdapter(
        std::string backendId)
        : backendId_(backendId)
    {
    }

    std::string backendId() const override
    {
        return backendId_;
    }

    IVdrTimerActionExecutor& executor() override
    {
        return executor_;
    }

    int callCount() const
    {
        return executor_.callCount();
    }

private:
    std::string backendId_;
    MockVdrTimerActionExecutor executor_;
};

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

static VdrTimerActionExecutorAdapterRegistry makeRegistry()
{
    VdrTimerActionExecutorAdapterRegistry registry;

    registry.registerAdapter(
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room"));

    return registry;
}

static BackendNode makeBackendNode(
    const std::string& backendId,
    const std::string& accessMode)
{
    BackendNode backend;
    backend.backendId = backendId;
    backend.backendName = backendId;
    backend.backendType = "restfulapi";
    backend.accessMode = accessMode;
    backend.enabled = true;
    backend.online = true;
    return backend;
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

static void test_create_body_uses_registry()
{
    VdrTimerActionExecutionService executionService;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;
    VdrTimerActionController controller(
        executionService,
        serializer,
        parser);

    VdrTimerActionExecutorAdapterRegistry registry =
        makeRegistry();

    const ApiResponse response =
        controller.createBody(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\""
            "}",
            registry);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"success\":true") != std::string::npos);
    assert(response.body.find("\"type\":\"create\"") != std::string::npos);
}

static void test_update_body_uses_registry()
{
    VdrTimerActionExecutionService executionService;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;
    VdrTimerActionController controller(
        executionService,
        serializer,
        parser);

    VdrTimerActionExecutorAdapterRegistry registry =
        makeRegistry();

    const ApiResponse response =
        controller.updateBody(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\""
            "}",
            registry);

    assert(response.statusCode == 200);
    assert(response.body.find("\"type\":\"update\"") != std::string::npos);
}

static void test_remove_body_uses_registry()
{
    VdrTimerActionExecutionService executionService;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;
    VdrTimerActionController controller(
        executionService,
        serializer,
        parser);

    VdrTimerActionExecutorAdapterRegistry registry =
        makeRegistry();

    const ApiResponse response =
        controller.removeBody(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\""
            "}",
            registry);

    assert(response.statusCode == 200);
    assert(response.body.find("\"type\":\"delete\"") != std::string::npos);
}

static void test_registry_body_allows_read_write_backend_with_policy()
{
    VdrTimerActionExecutionService executionService;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;

    BackendRegistry backendRegistry;
    backendRegistry.addBackend(
        makeBackendNode(
            "living-room",
            "read-write"));

    BackendRegistryService backendRegistryService(backendRegistry);
    BackendAccessPolicy backendAccessPolicy;

    VdrTimerActionController controller(
        executionService,
        serializer,
        parser,
        backendRegistryService,
        backendAccessPolicy);

    VdrTimerActionExecutorAdapterRegistry executorRegistry;
    auto adapter =
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room");

    executorRegistry.registerAdapter(adapter);

    const ApiResponse response =
        controller.createBody(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\""
            "}",
            executorRegistry);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"success\":true") != std::string::npos);
    assert(response.body.find("\"type\":\"create\"") != std::string::npos);
    assert(adapter->callCount() == 1);
}

static void test_registry_body_blocks_read_only_backend_with_policy()
{
    VdrTimerActionExecutionService executionService;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;

    BackendRegistry backendRegistry;
    backendRegistry.addBackend(
        makeBackendNode(
            "living-room",
            "read-only"));

    BackendRegistryService backendRegistryService(backendRegistry);
    BackendAccessPolicy backendAccessPolicy;

    VdrTimerActionController controller(
        executionService,
        serializer,
        parser,
        backendRegistryService,
        backendAccessPolicy);

    VdrTimerActionExecutorAdapterRegistry executorRegistry;
    auto adapter =
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room");

    executorRegistry.registerAdapter(adapter);

    const ApiResponse response =
        controller.updateBody(
            "{"
            "\"backendId\":\"living-room\","
            "\"timerId\":\"42\""
            "}",
            executorRegistry);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"success\":false") != std::string::npos);
    assert(response.body.find("\"type\":\"update\"") != std::string::npos);
    assert(response.body.find("backend is read-only") != std::string::npos);
    assert(adapter->callCount() == 0);
}

static void test_missing_registry_adapter_returns_json_failure()
{
    VdrTimerActionExecutionService executionService;
    VdrTimerActionResultJsonSerializer serializer;
    VdrTimerActionRequestParser parser;
    VdrTimerActionController controller(
        executionService,
        serializer,
        parser);

    VdrTimerActionExecutorAdapterRegistry registry;

    const ApiResponse response =
        controller.removeBody(
            "{"
            "\"backendId\":\"missing-backend\","
            "\"timerId\":\"42\""
            "}",
            registry);

    assert(response.statusCode == 200);
    assert(response.body.find("\"success\":false") != std::string::npos);
    assert(response.body.find("timer action executor adapter not found") != std::string::npos);
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
    test_create_body_uses_registry();
    test_update_body_uses_registry();
    test_remove_body_uses_registry();
    test_registry_body_allows_read_write_backend_with_policy();
    test_registry_body_blocks_read_only_backend_with_policy();
    test_missing_registry_adapter_returns_json_failure();

    return 0;
}
