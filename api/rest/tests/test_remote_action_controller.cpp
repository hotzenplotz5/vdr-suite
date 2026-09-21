#include "BackendRegistry.h"
#include "RemoteActionController.h"

#include <cassert>
#include <memory>

class ControllerExecutor : public IRemoteActionExecutor
{
public:
    RemoteActionResult execute(const RemoteActionRequest& request) const override
    {
        return RemoteActionResult::ok(request, "Remote action executed");
    }
};

namespace
{
BackendNode backend(
    const std::string& id,
    const std::string& accessMode,
    bool remoteCapability)
{
    BackendNode node;
    node.backendId = id;
    node.backendName = id;
    node.accessMode = accessMode;
    node.enabled = true;
    node.capabilities.remoteControl = remoteCapability;
    return node;
}
}

int main()
{
    BackendRegistry registry;
    registry.addBackend(backend("default", "read-write", true));
    registry.addBackend(backend("read-only", "read-only", true));
    registry.addBackend(backend("no-capability", "read-write", false));
    registry.addBackend(backend("no-executor", "read-write", true));

    BackendRegistryService registryService(registry);
    BackendAccessPolicy policy;
    RemoteActionExecutorRegistry executors;
    executors.registerExecutor("default", std::make_shared<ControllerExecutor>());
    RemoteActionService service(registryService, policy, executors);
    RemoteActionRequestParser parser;
    RemoteActionResultJsonSerializer serializer;
    int successfulCalls = 0;
    RemoteActionController controller(
        parser,
        service,
        serializer,
        [&successfulCalls](const RemoteActionRequest&) { ++successfulCalls; });

    ApiResponse response = controller.executeBody(
        "{\"backendId\":\"default\",\"operationId\":\"remote-1\",\"action\":\"ok\"}");
    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"success\":true") != std::string::npos);
    assert(response.body.find("\"failureKind\":\"none\"") != std::string::npos);
    assert(successfulCalls == 1);

    response = controller.executeBody("not-json");
    assert(response.statusCode == 400);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"failureKind\":\"validation\"") != std::string::npos);

    response = controller.executeBody(
        "{\"backendId\":\"default\",\"operationId\":\"remote-2\",\"action\":\"rawKey\"}");
    assert(response.statusCode == 400);
    assert(successfulCalls == 1);

    response = controller.executeBody(
        "{\"operationId\":\"remote-3\",\"action\":\"ok\"}");
    assert(response.statusCode == 400);

    response = controller.executeBody(
        "{\"backendId\":\"missing\",\"operationId\":\"remote-4\",\"action\":\"ok\"}");
    assert(response.statusCode == 404);
    assert(response.body.find("\"failureKind\":\"backendNotFound\"") != std::string::npos);

    response = controller.executeBody(
        "{\"backendId\":\"read-only\",\"operationId\":\"remote-5\",\"action\":\"ok\"}");
    assert(response.statusCode == 403);
    assert(response.body.find("\"failureKind\":\"permission\"") != std::string::npos);

    response = controller.executeBody(
        "{\"backendId\":\"no-capability\",\"operationId\":\"remote-6\",\"action\":\"ok\"}");
    assert(response.statusCode == 409);
    assert(response.body.find("\"failureKind\":\"capability\"") != std::string::npos);

    response = controller.executeBody(
        "{\"backendId\":\"no-executor\",\"operationId\":\"remote-7\",\"action\":\"ok\"}");
    assert(response.statusCode == 503);
    assert(response.body.find("\"failureKind\":\"executorUnavailable\"") != std::string::npos);

    response = controller.executeBody(
        "{\"backendId\":\"default\",\"operationId\":\"remote-8\",\"action\":\"switchChannel\"}");
    assert(response.statusCode == 400);

    response = controller.executeBody(
        "{\"backendId\":\"default\",\"operationId\":\"remote-9\",\"action\":\"ok\",\"channelId\":\"C-1\"}");
    assert(response.statusCode == 400);

    return 0;
}
