#include "RecordingActionSafetyService.h"

#include <cassert>
#include <string>

int main()
{
    RecordingActionSafetyService service;
    RecordingActionCapabilityContract capabilityContract;

    {
        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionCapabilitySet restfulApiCapabilities =
            capabilityContract.restfulApiDefaultCapabilities();

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilities(
                RecordingActionType::Delete,
                context,
                restfulApiCapabilities);

        assert(result.canExecute);
        assert(result.dryRun);
        assert(!result.missingCapability);
        assert(result.blockers.empty());
        assert(result.warnings.size() == 1);
        assert(result.warnings.at(0) == "dry-run only");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const RecordingActionCapabilitySet restfulApiCapabilities =
            capabilityContract.restfulApiDefaultCapabilities();

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilities(
                RecordingActionType::Cut,
                context,
                restfulApiCapabilities);

        assert(!result.canExecute);
        assert(result.missingCapability);
        assert(result.blockers.size() == 2);
        assert(result.blockers.at(0) ==
               "recording action capability is missing");
        assert(result.blockers.at(1) ==
               "missing capability: recording.action.cut");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.backendReadOnly = true;

        const RecordingActionCapabilitySet restfulApiCapabilities =
            capabilityContract.restfulApiDefaultCapabilities();

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilities(
                RecordingActionType::Move,
                context,
                restfulApiCapabilities);

        assert(!result.canExecute);
        assert(result.readOnlyBlocked);
        assert(!result.missingCapability);
        assert(result.blockers.size() == 1);
        assert(result.blockers.at(0) ==
               "recording action execution is blocked by read-only backend config");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.recordingInUse = true;

        const RecordingActionCapabilitySet restfulApiCapabilities =
            capabilityContract.restfulApiDefaultCapabilities();

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilities(
                RecordingActionType::Delete,
                context,
                restfulApiCapabilities);

        assert(!result.canExecute);
        assert(result.recordingInUse);
        assert(!result.missingCapability);
        assert(result.blockers.size() == 1);
        assert(result.blockers.at(0) ==
               "recording is currently in use");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const RecordingActionCapabilitySet liveCapabilities =
            capabilityContract.liveReferenceCapabilities();

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilities(
                RecordingActionType::Cut,
                context,
                liveCapabilities);

        assert(result.canExecute);
        assert(!result.missingCapability);
        assert(result.blockers.empty());
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        RecordingActionCapabilitySet emptyCapabilities;

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilities(
                RecordingActionType::Unknown,
                context,
                emptyCapabilities);

        assert(!result.canExecute);
        assert(result.unsupportedAction);
        assert(result.missingCapability);
        assert(result.blockers.size() == 3);
        assert(result.blockers.at(0) ==
               "recording action is not supported by backend");
        assert(result.blockers.at(1) ==
               "recording action capability is missing");
        assert(result.blockers.at(2) ==
               "missing capability: recording.action.unknown");
    }

    return 0;
}
