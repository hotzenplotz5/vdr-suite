#pragma once

#include "RecordingActionDispatchResult.h"
#include "RecordingActionExecutorSelectionResult.h"
#include "RecordingActionJobPayload.h"

class RecordingActionDispatchService
{
public:
    RecordingActionDispatchResult dispatch(
        const RecordingActionExecutorSelectionResult& selection,
        const RecordingActionJobPayload& payload) const
    {
        RecordingActionDispatchResult result;

        if (!selection.selected || !selection.executor)
        {
            result.dispatched = false;
            result.reason = "no executor selected";
            return result;
        }

        result.dispatched = true;
        result.executionResult = selection.executor->execute(payload);
        result.reason = "action dispatched";

        return result;
    }
};
