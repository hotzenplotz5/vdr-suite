#pragma once

#include "RecordingActionExecutionResult.h"
#include "RecordingActionJobPayload.h"

class IRecordingActionExecutor
{
public:
    virtual ~IRecordingActionExecutor() = default;

    virtual RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) = 0;
};
