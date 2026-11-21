#pragma once

#include "RecordingActionSafetyReason.h"

#include <string>
#include <vector>

struct RecordingActionSafetyResult
{
    bool canExecute = false;
    bool dryRun = true;
    bool readOnlyBlocked = false;
    bool executionDisabled = false;
    bool backendUnavailable = false;
    bool recordingInUse = false;
    bool missingCapability = false;
    bool unsupportedAction = false;

    std::vector<std::string> blockers;
    std::vector<RecordingActionSafetyReason> reasons;
    std::vector<std::string> warnings;

    bool hasBlockers() const
    {
        return !blockers.empty();
    }

    bool hasReasons() const
    {
        return !reasons.empty();
    }

    void addBlocker(
        RecordingActionSafetyReason reason,
        const std::string& blocker)
    {
        reasons.push_back(reason);
        blockers.push_back(blocker);
    }

    bool hasWarnings() const
    {
        return !warnings.empty();
    }

    static RecordingActionSafetyResult allowed(
        bool dryRunValue)
    {
        RecordingActionSafetyResult result;
        result.canExecute = true;
        result.dryRun = dryRunValue;

        if (dryRunValue) {
            result.warnings.push_back("dry-run only");
        }

        return result;
    }

    static RecordingActionSafetyResult blocked(
        bool dryRunValue,
        const std::string& blocker)
    {
        RecordingActionSafetyResult result;
        result.canExecute = false;
        result.dryRun = dryRunValue;
        result.addBlocker(
            RecordingActionSafetyReason::UnsupportedAction,
            blocker);
        return result;
    }
};
