#pragma once

#include "RecordingAction.h"
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
            result.blockers.push_back(
                "recording action is not supported by backend");
        }

        if (!context.backendAvailable) {
            result.backendUnavailable = true;
            result.blockers.push_back(
                "recording action backend is unavailable");
        }

        if (context.backendReadOnly) {
            result.readOnlyBlocked = true;
            result.blockers.push_back(
                "recording action execution is blocked by read-only backend config");
        }

        if (!context.capabilityAvailable) {
            result.missingCapability = true;
            result.blockers.push_back(
                "recording action capability is missing");
        }

        if (context.recordingInUse) {
            result.recordingInUse = true;
            result.blockers.push_back(
                "recording is currently in use");
        }

        if (!context.dryRun && !context.executionAllowed) {
            result.executionDisabled = true;
            result.blockers.push_back(
                "real recording action execution is disabled");
        }

        result.canExecute = !result.hasBlockers();

        return result;
    }
};
