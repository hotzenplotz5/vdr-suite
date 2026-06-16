#pragma once

#include "IRecordingActionExecutor.h"
#include "RecordingActionExecutionResult.h"
#include "RecordingActionJobPayloadFactory.h"
#include "RecordingActionRequest.h"
#include "RecordingActionValidationService.h"

class RecordingActionExecutionService
{
public:
    RecordingActionExecutionResult execute(
        const RecordingActionRequest& request,
        IRecordingActionExecutor& executor) const
    {
        const RecordingActionValidationResult validation =
            validationService_.validate(request);

        if (!validation.valid)
        {
            return RecordingActionExecutionResult::failed(
                request.type,
                request.recordingId,
                request.backendId,
                "recording action validation failed",
                validation.errors
            );
        }

        const RecordingActionJobPayload payload =
            payloadFactory_.create(request, validation);

        RecordingActionExecutionResult result =
            executor.execute(payload);

        if (!validation.warnings.empty())
        {
            result.warnings.insert(
                result.warnings.end(),
                validation.warnings.begin(),
                validation.warnings.end()
            );
        }

        return result;
    }

private:
    RecordingActionValidationService validationService_;
    RecordingActionJobPayloadFactory payloadFactory_;
};
