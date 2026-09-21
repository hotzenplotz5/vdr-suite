#include "RecordingActionValidationController.h"

#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionValidationService.h"

RecordingActionValidationController::RecordingActionValidationController(
    RecordingActionValidationService& validationService,
    RecordingActionValidationResultJsonSerializer& jsonSerializer)
    : validationService_(validationService),
      jsonSerializer_(jsonSerializer),
      requestParser_(nullptr)
{
}

RecordingActionValidationController::RecordingActionValidationController(
    RecordingActionValidationService& validationService,
    RecordingActionValidationResultJsonSerializer& jsonSerializer,
    RecordingActionValidationRequestParser& requestParser)
    : validationService_(validationService),
      jsonSerializer_(jsonSerializer),
      requestParser_(&requestParser)
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

ApiResponse RecordingActionValidationController::validateBody(
    const std::string& body)
{
    if (requestParser_ == nullptr)
    {
        ApiResponse response;

        response.statusCode = 500;
        response.contentType = "application/json";
        response.body = "{\"error\":\"recording action validation request parser unavailable\"}";

        return response;
    }

    return validate(
        requestParser_->parse(body));
}
