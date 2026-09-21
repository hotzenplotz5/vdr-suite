#include "RecordingActionPreviewController.h"

#include "RecordingActionValidationRequestParser.h"

RecordingActionPreviewController::RecordingActionPreviewController(
    RecordingActionRequestPreviewService& previewService,
    RecordingActionRequestPreviewResultJsonSerializer& jsonSerializer)
    : previewService_(previewService),
      jsonSerializer_(jsonSerializer),
      requestParser_(nullptr)
{
}

RecordingActionPreviewController::RecordingActionPreviewController(
    RecordingActionRequestPreviewService& previewService,
    RecordingActionRequestPreviewResultJsonSerializer& jsonSerializer,
    RecordingActionValidationRequestParser& requestParser)
    : previewService_(previewService),
      jsonSerializer_(jsonSerializer),
      requestParser_(&requestParser)
{
}

ApiResponse RecordingActionPreviewController::preview(
    const RecordingActionRequest& request,
    const RestfulApiRecordingActionBackendConfig& config)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(
            previewService_.previewRestfulApiRequest(
                request,
                config));

    return response;
}

ApiResponse RecordingActionPreviewController::previewBody(
    const std::string& body,
    const RestfulApiRecordingActionBackendConfig& config)
{
    if (requestParser_ == nullptr)
    {
        ApiResponse response;

        response.statusCode = 500;
        response.contentType = "application/json";
        response.body = "{\"error\":\"recording action preview request parser unavailable\"}";

        return response;
    }

    return preview(
        requestParser_->parse(body),
        config);
}
