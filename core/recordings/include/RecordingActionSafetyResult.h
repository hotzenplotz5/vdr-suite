#pragma once

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
    std::vector<std::string> warnings;

    bool hasBlockers() const
    {
        return !blockers.empty();
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
        result.blockers.push_back(blocker);
        return result;
    }
};
