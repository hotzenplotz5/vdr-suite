#pragma once

#include <string>
#include <vector>

enum class VdrTimerActionType
{
    Unknown,
    Create,
    Update,
    Delete,
    Toggle
};

struct VdrTimerActionResult
{
    bool success = false;
    VdrTimerActionType type = VdrTimerActionType::Unknown;
    std::string timerId;
    std::string backendId;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    static VdrTimerActionResult ok(
        VdrTimerActionType type,
        const std::string& timerId,
        const std::string& backendId,
        const std::string& message)
    {
        VdrTimerActionResult result;
        result.success = true;
        result.type = type;
        result.timerId = timerId;
        result.backendId = backendId;
        result.message = message;
        return result;
    }

    static VdrTimerActionResult failed(
        VdrTimerActionType type,
        const std::string& timerId,
        const std::string& backendId,
        const std::string& message,
        const std::vector<std::string>& errors)
    {
        VdrTimerActionResult result;
        result.success = false;
        result.type = type;
        result.timerId = timerId;
        result.backendId = backendId;
        result.message = message;
        result.errors = errors;
        return result;
    }

    bool hasWarnings() const
    {
        return !warnings.empty();
    }

    bool hasErrors() const
    {
        return !errors.empty();
    }
};
