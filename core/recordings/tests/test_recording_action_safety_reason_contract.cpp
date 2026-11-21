#include "RecordingActionSafetyService.h"

#include <cassert>
#include <string>

int main()
{
    assert(toString(RecordingActionSafetyReason::Allowed) == "allowed");
    assert(toString(RecordingActionSafetyReason::UnsupportedAction) == "unsupported_action");
    assert(toString(RecordingActionSafetyReason::BackendUnavailable) == "backend_unavailable");
    assert(toString(RecordingActionSafetyReason::BackendReadOnly) == "backend_read_only");
    assert(toString(RecordingActionSafetyReason::MissingCapability) == "missing_capability");
    assert(toString(RecordingActionSafetyReason::RecordingInUse) == "recording_in_use");
    assert(toString(RecordingActionSafetyReason::ExecutionDisabled) == "execution_disabled");

    RecordingActionSafetyService service;

    {
        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Delete, context);

        assert(result.canExecute);
        assert(!result.hasBlockers());
        assert(!result.hasReasons());
        assert(result.hasWarnings());
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.backendAvailable = false;
        context.backendReadOnly = true;
        context.capabilityAvailable = false;
        context.recordingInUse = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluate(RecordingActionType::Unknown, context);

        assert(!result.canExecute);
        assert(result.unsupportedAction);
        assert(result.backendUnavailable);
        assert(result.readOnlyBlocked);
        assert(result.missingCapability);
        assert(result.recordingInUse);
        assert(result.executionDisabled);

        assert(result.reasons.size() == 6);
        assert(result.reasons.at(0) == RecordingActionSafetyReason::UnsupportedAction);
        assert(result.reasons.at(1) == RecordingActionSafetyReason::BackendUnavailable);
        assert(result.reasons.at(2) == RecordingActionSafetyReason::BackendReadOnly);
        assert(result.reasons.at(3) == RecordingActionSafetyReason::MissingCapability);
        assert(result.reasons.at(4) == RecordingActionSafetyReason::RecordingInUse);
        assert(result.reasons.at(5) == RecordingActionSafetyReason::ExecutionDisabled);
    }

    {
        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        RecordingActionCapabilitySet capabilitySet;

        const RecordingActionSafetyResult result =
            service.evaluateWithCapabilities(
                RecordingActionType::Move,
                context,
                capabilitySet);

        assert(!result.canExecute);
        assert(result.missingCapability);
        assert(result.reasons.size() == 1);
        assert(result.reasons.at(0) == RecordingActionSafetyReason::MissingCapability);
        assert(result.blockers.size() == 2);
        assert(result.blockers.at(0) == "recording action capability is missing");
        assert(result.blockers.at(1) == "missing capability: recording.action.move");
    }

    return 0;
}
