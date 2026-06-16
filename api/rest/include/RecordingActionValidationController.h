#pragma once

#include "DashboardController.h"
#include "RecordingActionRequest.h"

class RecordingActionValidationResultJsonSerializer;
class RecordingActionValidationService;

class RecordingActionValidationController
{
public:
    RecordingActionValidationController(
        RecordingActionValidationService& validationService,
        RecordingActionValidationResultJsonSerializer& jsonSerializer);

    ApiResponse validate(
        const RecordingActionRequest& request);

private:
    RecordingActionValidationService& validationService_;
    RecordingActionValidationResultJsonSerializer& jsonSerializer_;
};
