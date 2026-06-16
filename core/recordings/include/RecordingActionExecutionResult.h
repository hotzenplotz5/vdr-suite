#pragma once

#include <string>
#include <vector>

struct RecordingActionExecutionResult {
    bool success = false;
    std::string action;
    std::string recordingId;
    std::string backendId;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    static RecordingActionExecutionResult ok(
        const std::string& action,
        const std::string& recordingId,
        const std::string& backendId,
        const std::string& message
    ) {
        RecordingActionExecutionResult result;
        result.success = true;
        result.action = action;
        result.recordingId = recordingId;
        result.backendId = backendId;
        result.message = message;
        return result;
    }

    static RecordingActionExecutionResult failed(
        const std::string& action,
        const std::string& recordingId,
        const std::string& backendId,
        const std::string& message,
        const std::vector<std::string>& errors
    ) {
        RecordingActionExecutionResult result;
        result.success = false;
        result.action = action;
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
