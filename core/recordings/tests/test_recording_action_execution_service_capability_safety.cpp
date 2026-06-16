#include "RecordingActionExecutionService.h"

#include <cassert>

int main()
{
    RecordingActionExecutionService service;
    RecordingActionCapabilityContract capabilityContract;

    {
        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(
                RecordingActionType::Delete,
                context,
                capabilityContract.restfulApiDefaultCapabilities());

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

        const RecordingActionSafetyResult result =
            service.evaluateSafety(
                RecordingActionType::Cut,
                context,
                capabilityContract.restfulApiDefaultCapabilities());

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

        const RecordingActionSafetyResult result =
            service.evaluateSafety(
                RecordingActionType::Move,
                context,
                capabilityContract.restfulApiDefaultCapabilities());

        assert(!result.canExecute);
        assert(result.readOnlyBlocked);
        assert(!result.missingCapability);
        assert(result.blockers.size() == 1);
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(
                RecordingActionType::Cut,
                context,
                capabilityContract.liveReferenceCapabilities());

        assert(result.canExecute);
        assert(!result.missingCapability);
        assert(result.blockers.empty());
    }

    return 0;
}
