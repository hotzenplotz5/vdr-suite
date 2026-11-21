#include "RecordingActionSafetyService.h"

#include <cassert>
#include <string>

int main()
{
    RecordingActionSafetyService service;

    {
        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Delete, context);

        assert(result.canExecute);
        assert(result.dryRun);
        assert(result.hasWarnings());
        assert(result.warnings.at(0) == "dry-run only");
        assert(!result.hasBlockers());
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Move, context);

        assert(result.canExecute);
        assert(!result.dryRun);
        assert(!result.hasWarnings());
        assert(!result.hasBlockers());
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.backendReadOnly = true;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Delete, context);

        assert(!result.canExecute);
        assert(result.readOnlyBlocked);
        assert(result.hasBlockers());
        assert(result.blockers.at(0) ==
               "recording action execution is blocked by read-only backend config");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Rename, context);

        assert(!result.canExecute);
        assert(result.executionDisabled);
        assert(result.hasBlockers());
        assert(result.blockers.at(0) ==
               "real recording action execution is disabled");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.backendAvailable = false;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Move, context);

        assert(!result.canExecute);
        assert(result.backendUnavailable);
        assert(result.blockers.at(0) ==
               "recording action backend is unavailable");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.capabilityAvailable = false;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Move, context);

        assert(!result.canExecute);
        assert(result.missingCapability);
        assert(result.blockers.at(0) ==
               "recording action capability is missing");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.recordingInUse = true;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Delete, context);

        assert(!result.canExecute);
        assert(result.recordingInUse);
        assert(result.blockers.at(0) ==
               "recording is currently in use");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.actionSupported = false;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::MetadataRefresh, context);

        assert(!result.canExecute);
        assert(result.unsupportedAction);
        assert(result.blockers.at(0) ==
               "recording action is not supported by backend");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = false;
        context.backendReadOnly = true;
        context.recordingInUse = true;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Delete, context);

        assert(!result.canExecute);
        assert(result.readOnlyBlocked);
        assert(result.executionDisabled);
        assert(result.recordingInUse);
        assert(result.blockers.size() == 3);
    }

    return 0;
}
