#pragma once

#include "RecordingAction.h"
#include "RecordingActionCapabilityContract.h"
#include "RecordingActionSafetyResult.h"

#include <string>

struct RecordingActionSafetyContext
{
    bool dryRun = true;
    bool backendAvailable = true;
    bool backendReadOnly = false;
    bool executionAllowed = false;
    bool capabilityAvailable = true;
    bool recordingInUse = false;
    bool actionSupported = true;
};

class RecordingActionSafetyService
{
public:
    RecordingActionSafetyResult evaluateWithCapabilities(
        RecordingActionType action,
        const RecordingActionSafetyContext& context,
        const RecordingActionCapabilitySet& capabilitySet) const
    {
        RecordingActionSafetyContext capabilityAwareContext = context;

        const RecordingActionCapabilityCheckResult capabilityCheck =
            capabilityContract_.check(action, capabilitySet);

        capabilityAwareContext.capabilityAvailable =
            capabilityAwareContext.capabilityAvailable &&
            capabilityCheck.supported;

        RecordingActionSafetyResult result =
            evaluate(action, capabilityAwareContext);

        if (!capabilityCheck.supported)
        {
            for (const std::string& missingCapability :
                 capabilityCheck.missingCapabilities)
            {
                result.blockers.push_back(
                    "missing capability: " + missingCapability);
            }

            result.canExecute = false;
        }

        return result;
    }

    RecordingActionSafetyResult evaluate(
        RecordingActionType action,
        const RecordingActionSafetyContext& context) const
    {
        RecordingActionSafetyResult result;
        result.dryRun = context.dryRun;

        if (context.dryRun) {
            result.warnings.push_back("dry-run only");
        }

        if (action == RecordingActionType::Unknown ||
            !context.actionSupported) {
            result.unsupportedAction = true;
            result.addBlocker(
                RecordingActionSafetyReason::UnsupportedAction,
                "recording action is not supported by backend");
        }

        if (!context.backendAvailable) {
            result.backendUnavailable = true;
            result.addBlocker(
                RecordingActionSafetyReason::BackendUnavailable,
                "recording action backend is unavailable");
        }

        if (context.backendReadOnly) {
            result.readOnlyBlocked = true;
            result.addBlocker(
                RecordingActionSafetyReason::BackendReadOnly,
                "recording action execution is blocked by read-only backend config");
        }

        if (!context.capabilityAvailable) {
            result.missingCapability = true;
            result.addBlocker(
                RecordingActionSafetyReason::MissingCapability,
                "recording action capability is missing");
        }

        if (context.recordingInUse) {
            result.recordingInUse = true;
            result.addBlocker(
                RecordingActionSafetyReason::RecordingInUse,
                "recording is currently in use");
        }

        if (!context.dryRun && !context.executionAllowed) {
            result.executionDisabled = true;
            result.addBlocker(
                RecordingActionSafetyReason::ExecutionDisabled,
                "real recording action execution is disabled");
        }

        result.canExecute = !result.hasBlockers();

        return result;
    }

private:
    RecordingActionCapabilityContract capabilityContract_;
};
