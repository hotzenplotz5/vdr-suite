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
      backendRegistry_(nullptr),
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
      backendRegistry_(nullptr),
      requestParser_(&requestParser)
{
}

RecordingActionExecutionController::RecordingActionExecutionController(
    RecordingActionExecutionService& executionService,
    RecordingActionExecutionResultJsonSerializer& jsonSerializer,
    RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry,
    BackendRegistry& backendRegistry,
    RecordingActionValidationRequestParser& requestParser)
    : executionService_(executionService),
      jsonSerializer_(jsonSerializer),
      backendExecutorAdapterRegistry_(backendExecutorAdapterRegistry),
      backendRegistry_(&backendRegistry),
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

ApiResponse RecordingActionExecutionController::safety(
    const RecordingActionRequest& request,
    const RecordingActionSafetyContext& context)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    if (backendRegistry_ != nullptr)
    {
        const RecordingActionBackendPolicyLookupResult lookup =
            backendPolicyProvider_.policyForBackend(
                *backendRegistry_,
                request.backendId);

        response.body =
            safetyJsonSerializer_.serialize(
                executionService_.evaluateSafety(
                    request,
                    lookup.policy));

        return response;
    }

    response.body =
        safetyJsonSerializer_.serialize(
            executionService_.evaluateSafety(
                request,
                context,
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
