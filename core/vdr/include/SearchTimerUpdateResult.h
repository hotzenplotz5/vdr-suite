#pragma once

#include "SearchTimer.h"

#include <string>
#include <vector>

struct SearchTimerUpdateResult
{
    bool success = false;
    SearchTimer searchTimer;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    static SearchTimerUpdateResult ok(
        const SearchTimer& updatedSearchTimer,
        const std::string& message)
    {
        SearchTimerUpdateResult result;
        result.success = true;
        result.searchTimer = updatedSearchTimer;
        result.message = message;
        return result;
    }

    static SearchTimerUpdateResult failed(
        const std::string& message,
        const std::vector<std::string>& errors)
    {
        SearchTimerUpdateResult result;
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
