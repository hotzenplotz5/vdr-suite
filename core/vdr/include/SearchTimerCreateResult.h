#pragma once

#include "SearchTimer.h"

#include <string>
#include <vector>

struct SearchTimerCreateResult
{
    bool success = false;
    SearchTimer searchTimer;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    static SearchTimerCreateResult ok(
        const SearchTimer& createdSearchTimer,
        const std::string& message)
    {
        SearchTimerCreateResult result;
        result.success = true;
        result.searchTimer = createdSearchTimer;
        result.message = message;
        return result;
    }

    static SearchTimerCreateResult failed(
        const std::string& message,
        const std::vector<std::string>& errors)
    {
        SearchTimerCreateResult result;
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
