#pragma once

#include "RecordingActionRequest.h"
#include "RecordingActionValidationResult.h"

class RecordingActionValidationService
{
public:
    RecordingActionValidationResult validate(
        const RecordingActionRequest& request) const;
};
