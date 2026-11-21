#include "RecordingActionExecutionService.h"

#include <cassert>

int main()
{
    RecordingActionExecutionService service;

    {
        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(RecordingActionType::Delete, context);

        assert(result.canExecute);
        assert(result.dryRun);
        assert(result.warnings.size() == 1);
        assert(result.warnings.at(0) == "dry-run only");
        assert(result.blockers.empty());
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.backendReadOnly = true;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(RecordingActionType::Delete, context);

        assert(!result.canExecute);
        assert(result.readOnlyBlocked);
        assert(result.blockers.size() == 1);
        assert(result.blockers.at(0) ==
               "recording action execution is blocked by read-only backend config");
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = false;
        context.recordingInUse = true;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(RecordingActionType::Move, context);

        assert(!result.canExecute);
        assert(result.executionDisabled);
        assert(result.recordingInUse);
        assert(result.blockers.size() == 2);
    }

    return 0;
}
