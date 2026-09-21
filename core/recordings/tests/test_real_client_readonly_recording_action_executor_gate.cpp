#include "BasicHttpClient.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionBackendPolicy.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionPermissionContract.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>
#include <memory>

int main()
{
    BasicHttpClient httpClient("127.0.0.1", 8002);

    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "local-vdr";
    config.host = "127.0.0.1";
    config.port = 8002;
    config.basePath = "";
    config.readOnly = true;
    config.allowExecution = false;

    auto adapter =
        std::make_shared<RestfulApiRecordingActionBackendExecutorAdapter>(
            config,
            httpClient);

    RecordingActionBackendExecutorAdapterRegistry registry;
    registry.registerAdapter(adapter);

    RecordingActionPermissionContract permissionContract;

    RecordingActionBackendPolicy policy;
    policy.backendId = "local-vdr";
    policy.backendAvailable = true;
    policy.readOnly = true;
    policy.executionAllowed = false;
    policy.capabilities = adapter->capabilities();
    policy.permissions = permissionContract.readOnlyPermissions();

    RecordingActionRequest request;
    request.backendId = "local-vdr";
    request.recordingId =
        "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec";
    request.type = RecordingActionType::Delete;
    request.dryRun = false;
    request.parameters["recordingPath"] =
        "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec";

    RecordingActionExecutionService service;

    const RecordingActionExecutionResult result =
        service.execute(request, registry, policy);

    assert(!result.success);
    assert(result.backendId == "local-vdr");
    assert(result.type == RecordingActionType::Delete);
    assert(result.message == "recording action execution blocked by safety policy");

    bool hasReadOnlyBlocker = false;
    bool hasPermissionBlocker = false;

    for (const std::string& error : result.errors)
    {
        if (error == "recording action execution is blocked by read-only backend config")
        {
            hasReadOnlyBlocker = true;
        }

        if (error == "missing permission: recording.permission.delete")
        {
            hasPermissionBlocker = true;
        }
    }

    assert(hasReadOnlyBlocker);
    assert(hasPermissionBlocker);

    return 0;
}
