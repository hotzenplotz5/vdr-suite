#include "BackendRegistry.h"
#include "RemoteActionService.h"

#include <cassert>
#include <memory>

class SuccessfulExecutor : public IRemoteActionExecutor
{
public:
    RemoteActionResult execute(const RemoteActionRequest& request) const override
    {
        return RemoteActionResult::ok(request, "executed");
    }
};

BackendNode backend(const std::string& id, const std::string& accessMode, bool capability)
{
    BackendNode node;
    node.backendId = id;
    node.backendName = id;
    node.accessMode = accessMode;
    node.enabled = true;
    node.capabilities.remoteControl = capability;
    return node;
}

RemoteActionRequest action(const std::string& backendId)
{
    RemoteActionRequest request;
    request.backendId = backendId;
    request.operationId = "op-1";
    request.action = RemoteActionType::Ok;
    return request;
}

int main()
{
    BackendRegistry registry;
    registry.addBackend(backend("write", "read-write", true));
    registry.addBackend(backend("read-only", "read-only", true));
    registry.addBackend(backend("no-capability", "read-write", false));
    BackendRegistryService registryService(registry);
    BackendAccessPolicy accessPolicy;
    RemoteActionExecutorRegistry executors;
    executors.registerExecutor("write", std::make_shared<SuccessfulExecutor>());
    RemoteActionService service(registryService, accessPolicy, executors);

    assert(service.execute(action("write")).success);
    assert(service.execute(action("missing")).failureKind == RemoteActionFailureKind::BackendNotFound);
    assert(service.execute(action("read-only")).failureKind == RemoteActionFailureKind::Permission);
    assert(service.execute(action("no-capability")).failureKind == RemoteActionFailureKind::Capability);

    RemoteActionRequest invalid = action("write");
    invalid.operationId.clear();
    assert(service.execute(invalid).failureKind == RemoteActionFailureKind::Validation);

    registry.addBackend(backend("no-executor", "read-write", true));
    assert(service.execute(action("no-executor")).failureKind == RemoteActionFailureKind::ExecutorUnavailable);
    return 0;
}
