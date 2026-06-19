#include "RecordingActionExecutionController.h"

#include "RecordingActionExecutionResult.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationRequestParser.h"
#include "VdrSnapshotReadService.h"

RecordingActionExecutionController::RecordingActionExecutionController(
    RecordingActionExecutionService& executionService,
    RecordingActionExecutionResultJsonSerializer& jsonSerializer,
    RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry)
    : executionService_(executionService),
      jsonSerializer_(jsonSerializer),
      backendExecutorAdapterRegistry_(backendExecutorAdapterRegistry),
      backendRegistry_(nullptr),
      requestParser_(nullptr),
      snapshotReadService_(nullptr)
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
      requestParser_(&requestParser),
      snapshotReadService_(nullptr)
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
      requestParser_(&requestParser),
      snapshotReadService_(nullptr)
{
}

RecordingActionExecutionController::RecordingActionExecutionController(
    RecordingActionExecutionService& executionService,
    RecordingActionExecutionResultJsonSerializer& jsonSerializer,
    RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry,
    BackendRegistry& backendRegistry,
    RecordingActionValidationRequestParser& requestParser,
    VdrSnapshotReadService& snapshotReadService)
    : executionService_(executionService),
      jsonSerializer_(jsonSerializer),
      backendExecutorAdapterRegistry_(backendExecutorAdapterRegistry),
      backendRegistry_(&backendRegistry),
      requestParser_(&requestParser),
      snapshotReadService_(&snapshotReadService)
{
}

void RecordingActionExecutionController::setAfterSuccessfulExecutionCallback(
    std::function<void()> callback)
{
    afterSuccessfulExecution_ = std::move(callback);
}

void RecordingActionExecutionController::refreshAfterSuccessfulExecution(
    const RecordingActionExecutionResult& result) const
{
    if (!result.success)
    {
        return;
    }

    if (!afterSuccessfulExecution_)
    {
        return;
    }

    afterSuccessfulExecution_();
}

RecordingActionRequest RecordingActionExecutionController::resolveBackendNativeId(
    const RecordingActionRequest& request) const
{
    if (snapshotReadService_ == nullptr)
    {
        return request;
    }

    if (request.parameters.find("backendNativeId") != request.parameters.end())
    {
        return request;
    }

    if (request.backendId.empty() || request.recordingId.empty())
    {
        return request;
    }

    RecordingActionRequest resolved = request;

    const std::vector<VdrRecording> recordings =
        snapshotReadService_->getRecordingsForBackend(request.backendId);

    for (const VdrRecording& recording : recordings)
    {
        if (recording.id == request.recordingId && !recording.backendNativeId.empty())
        {
            resolved.parameters["backendNativeId"] = recording.backendNativeId;
            return resolved;
        }
    }

    return resolved;
}

ApiResponse RecordingActionExecutionController::execute(
    const RecordingActionRequest& request)
{
    ApiResponse response;

    const RecordingActionRequest resolvedRequest =
        resolveBackendNativeId(request);

    response.statusCode = 200;
    response.contentType = "application/json";
    if (backendRegistry_ != nullptr)
    {
        const RecordingActionBackendPolicyLookupResult lookup =
            backendPolicyProvider_.policyForBackend(
                *backendRegistry_,
                request.backendId);

        const RecordingActionExecutionResult result =
            executionService_.execute(
                resolvedRequest,
                backendExecutorAdapterRegistry_,
                lookup.policy);

        refreshAfterSuccessfulExecution(result);

        response.body =
            jsonSerializer_.serialize(result);

        return response;
    }

    const RecordingActionExecutionResult result =
        executionService_.execute(
            resolvedRequest,
            backendExecutorAdapterRegistry_);

    refreshAfterSuccessfulExecution(result);

    response.body =
        jsonSerializer_.serialize(result);

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
