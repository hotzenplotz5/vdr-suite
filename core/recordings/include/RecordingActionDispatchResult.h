#pragma once

#include "RecordingActionExecutionResult.h"

struct RecordingActionDispatchResult
{
    bool dispatched = false;

    RecordingActionExecutionResult executionResult;

    std::string reason;
};
