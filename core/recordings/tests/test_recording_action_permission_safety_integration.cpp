#include "RecordingActionSafetyService.h"

#include <cassert>

int main()
{
    RecordingActionSafetyService service;
    RecordingActionCapabilityContract capabilityContract;
    RecordingActionPermissionContract permissionContract;

    {
        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluateWithPermissions(
                RecordingActionType::Move,
                context,
                permissionContract.readOnlyPermissions());

        assert(!result.canExecute);
        assert(result.reasons.size() == 1);
        assert(result.reasons.at(0) ==
               RecordingActionSafetyReason::PermissionDenied);
        assert(result.blockers.size() == 2);
        assert(result.blockers.at(0) ==
               "recording action permission is denied");
        assert(result.blockers.at(1) ==
               "missing permission: recording.permission.move");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluateWithPermissions(
                RecordingActionType::Move,
                context,
                permissionContract.restfulApiMutationPermissions());

        assert(result.canExecute);
        assert(result.reasons.empty());
        assert(result.blockers.empty());
        assert(result.warnings.size() == 1);
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

        assert(!result.canExecute);
        assert(result.missingCapability);
        assert(result.reasons.size() == 2);
        assert(result.reasons.at(0) ==
               RecordingActionSafetyReason::MissingCapability);
        assert(result.reasons.at(1) ==
               RecordingActionSafetyReason::PermissionDenied);
        assert(result.blockers.at(0) ==
               "recording action capability is missing");
        assert(result.blockers.at(1) ==
               "missing capability: recording.action.cut");
        assert(result.blockers.at(2) ==
               "recording action permission is denied");
        assert(result.blockers.at(3) ==
               "missing permission: recording.permission.cut");
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

        assert(result.canExecute);
        assert(result.reasons.empty());
        assert(result.blockers.empty());
    }

    return 0;
}
