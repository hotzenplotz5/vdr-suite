#include "RecordingActionSafetyResultJsonSerializer.h"
#include "RecordingActionSafetyService.h"

#include <cassert>
#include <string>

int main()
{
    RecordingActionSafetyService service;
    RecordingActionSafetyResultJsonSerializer serializer;
    RecordingActionPermissionContract permissionContract;
    RecordingActionCapabilityContract capabilityContract;

    {
        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluateWithPermissions(
                RecordingActionType::Move,
                context,
                permissionContract.readOnlyPermissions());

        const std::string json =
            serializer.serialize(result);

        assert(json.find("\"canExecute\":false") != std::string::npos);
        assert(json.find("\"dryRun\":true") != std::string::npos);
        assert(json.find("\"reasons\":[\"permission_denied\"]") != std::string::npos);
        assert(json.find("recording action permission is denied") != std::string::npos);
        assert(json.find("missing permission: recording.permission.move") != std::string::npos);
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilitiesAndPermissions(
                RecordingActionType::Cut,
                context,
                capabilityContract.restfulApiDefaultCapabilities(),
                permissionContract.readOnlyPermissions());

        const std::string json =
            serializer.serialize(result);

        assert(json.find("\"canExecute\":false") != std::string::npos);
        assert(json.find("\"dryRun\":false") != std::string::npos);
        assert(json.find("\"missingCapability\":true") != std::string::npos);
        assert(json.find("\"reasons\":[\"missing_capability\",\"permission_denied\"]") != std::string::npos);
        assert(json.find("missing capability: recording.action.cut") != std::string::npos);
        assert(json.find("missing permission: recording.permission.cut") != std::string::npos);
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilitiesAndPermissions(
                RecordingActionType::Move,
                context,
                capabilityContract.restfulApiDefaultCapabilities(),
                permissionContract.restfulApiMutationPermissions());

        const std::string json =
            serializer.serialize(result);

        assert(json.find("\"canExecute\":true") != std::string::npos);
        assert(json.find("\"reasons\":[]") != std::string::npos);
        assert(json.find("\"blockers\":[]") != std::string::npos);
    }

    return 0;
}
