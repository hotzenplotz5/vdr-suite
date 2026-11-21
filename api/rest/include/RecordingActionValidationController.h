#pragma once

#include "DashboardController.h"
#include "RecordingActionRequest.h"

#include <string>

class RecordingActionValidationRequestParser;
class RecordingActionValidationResultJsonSerializer;
class RecordingActionValidationService;
struct RecordingActionValidationResult;

class RecordingActionValidationController
{
public:
    RecordingActionValidationController(
        RecordingActionValidationService& validationService,
        RecordingActionValidationResultJsonSerializer& jsonSerializer);

    RecordingActionValidationController(
        RecordingActionValidationService& validationService,
        RecordingActionValidationResultJsonSerializer& jsonSerializer,
        RecordingActionValidationRequestParser& requestParser);

    ApiResponse validate(
        const RecordingActionRequest& request);

    RecordingActionValidationResult validateResult(
        const RecordingActionRequest& request) const;

    ApiResponse validateBody(
        const std::string& body);

private:
    RecordingActionValidationService& validationService_;
    RecordingActionValidationResultJsonSerializer& jsonSerializer_;
    RecordingActionValidationRequestParser* requestParser_;
};
