#pragma once

#include "RecordingAction.h"

#include <string>
#include <vector>

struct RecordingActionExecutionResult {
    bool success = false;
    RecordingActionType type = RecordingActionType::Unknown;
    std::string recordingId;
    std::string backendId;
    std::string backendNativeId;
    std::string recordingPath;
    bool snapshotRefreshed = false;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    static RecordingActionExecutionResult ok(
        RecordingActionType type,
        const std::string& recordingId,
        const std::string& backendId,
        const std::string& message
    ) {
        RecordingActionExecutionResult result;
        result.success = true;
        result.type = type;
        result.recordingId = recordingId;
        result.backendId = backendId;
        result.message = message;
        return result;
    }

    static RecordingActionExecutionResult failed(
        RecordingActionType type,
        const std::string& recordingId,
        const std::string& backendId,
        const std::string& message,
        const std::vector<std::string>& errors
    ) {
        RecordingActionExecutionResult result;
        result.success = false;
        result.type = type;
        result.recordingId = recordingId;
        result.backendId = backendId;
        result.message = message;
        result.errors = errors;
        return result;
    }

    bool hasWarnings() const {
        return !warnings.empty();
    }

    bool hasErrors() const {
        return !errors.empty();
    }
};
