#include "RecordingActionExecutionController.h"

#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationRequestParser.h"

RecordingActionExecutionController::RecordingActionExecutionController(
    RecordingActionExecutionService& executionService,
    RecordingActionExecutionResultJsonSerializer& jsonSerializer,
    RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry)
    : executionService_(executionService),
      jsonSerializer_(jsonSerializer),
      backendExecutorAdapterRegistry_(backendExecutorAdapterRegistry),
      requestParser_(nullptr)
{
}

RecordingActionExecutionController::RecordingActionExecutionController(
    RecordingActionExecutionService& executionService,
    RecordingActionExecutionResultJsonSerializer& jsonSerializer,
    RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry,
    RecordingActionValidationRequestParser& requestParser)
    : executionService_(executionService),
      jsonSerializer_(jsonSerializer),
      backendExecutorAdapterRegistry_(backendExecutorAdapterRegistry),
      requestParser_(&requestParser)
{
}

ApiResponse RecordingActionExecutionController::execute(
    const RecordingActionRequest& request)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(
            executionService_.execute(
                request,
                backendExecutorAdapterRegistry_));

    return response;
}

ApiResponse RecordingActionExecutionController::executeBody(
    const std::string& body)
{
    if (requestParser_ == nullptr)
    {
        ApiResponse response;

        response.statusCode = 500;
        response.contentType = "application/json";
        response.body = "{\"error\":\"recording action execution request parser unavailable\"}";

        return response;
    }

    return execute(
        requestParser_->parse(body));
}
