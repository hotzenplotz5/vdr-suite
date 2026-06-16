#pragma once

#include "RecordingActionExecutionResult.h"
#include "RecordingActionRequest.h"

class IRecordingActionExecutor
{
public:
    virtual ~IRecordingActionExecutor() = default;

    virtual RecordingActionExecutionResult execute(
        const RecordingActionRequest& request) = 0;
};
