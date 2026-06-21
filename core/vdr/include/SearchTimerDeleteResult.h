#pragma once

#include <string>
#include <vector>

struct SearchTimerDeleteResult
{
    bool success = false;
    std::string backendId;
    std::string backendNativeId;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    static SearchTimerDeleteResult ok(
        const std::string& deletedBackendId,
        const std::string& deletedBackendNativeId,
        const std::string& message)
    {
        SearchTimerDeleteResult result;
        result.success = true;
        result.backendId = deletedBackendId;
        result.backendNativeId = deletedBackendNativeId;
        result.message = message;
        return result;
    }

    static SearchTimerDeleteResult failed(
        const std::string& message,
        const std::vector<std::string>& errors)
    {
        SearchTimerDeleteResult result;
        result.success = false;
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
