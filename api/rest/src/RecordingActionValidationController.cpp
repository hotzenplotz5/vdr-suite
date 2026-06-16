#include "RecordingActionValidationController.h"

#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionValidationService.h"

RecordingActionValidationController::RecordingActionValidationController(
    RecordingActionValidationService& validationService,
    RecordingActionValidationResultJsonSerializer& jsonSerializer)
    : validationService_(validationService),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse RecordingActionValidationController::validate(
    const RecordingActionRequest& request)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(
            validationService_.validate(request));

    return response;
}
