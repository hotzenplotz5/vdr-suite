#include "RecordingActionExecutionController.h"

#include "RecordingActionExecutionResult.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"
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
    std::function<bool(const RecordingActionRequest&)> callback)
{
    afterSuccessfulExecution_ = std::move(callback);
}

bool RecordingActionExecutionController::refreshAfterSuccessfulExecution(
    const RecordingActionExecutionResult& result,
    const RecordingActionRequest& resolvedRequest) const
{
    if (!result.success || resolvedRequest.dryRun)
    {
        return false;
    }

    if (!afterSuccessfulExecution_)
    {
        return false;
    }

    try
    {
        return afterSuccessfulExecution_(resolvedRequest);
    }
    catch (...)
    {
        return false;
    }
}

RecordingActionExecutionResult RecordingActionExecutionController::enrichExecutionResult(
    RecordingActionExecutionResult result,
    const RecordingActionRequest& resolvedRequest,
    bool snapshotRefreshed) const
{
    const auto backendNativeId =
        resolvedRequest.parameters.find("backendNativeId");
    if (backendNativeId != resolvedRequest.parameters.end())
    {
        result.backendNativeId = backendNativeId->second;
    }

    const auto recordingPath =
        resolvedRequest.parameters.find("recordingPath");
    if (recordingPath != resolvedRequest.parameters.end())
    {
        result.recordingPath = recordingPath->second;
    }

    result.snapshotRefreshed = snapshotRefreshed;

    return result;
}

RecordingActionRequest RecordingActionExecutionController::resolveBackendNativeId(
    const RecordingActionRequest& request) const
{
    if (snapshotReadService_ == nullptr)
    {
        return request;
    }

    const auto backendNativeId =
        request.parameters.find("backendNativeId");
    const bool needsBackendNativeId =
        backendNativeId == request.parameters.end() ||
        backendNativeId->second.empty();

    const auto recordingTitle =
        request.parameters.find("recordingTitle");
    const bool needsRecordingTitle =
        recordingTitle == request.parameters.end() ||
        recordingTitle->second.empty();

    if (!needsBackendNativeId && !needsRecordingTitle)
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
        if (recording.id != request.recordingId)
        {
            continue;
        }

        if (needsBackendNativeId &&
            !recording.backendNativeId.empty())
        {
            resolved.parameters["backendNativeId"] =
                recording.backendNativeId;
        }

        if (needsRecordingTitle &&
            !recording.title.empty())
        {
            resolved.parameters["recordingTitle"] =
                recording.title;
        }

        return resolved;
    }

    return resolved;
}

RecordingActionExecutionResult RecordingActionExecutionController::previewBackend(
    const RecordingActionRequest& request) const
{
    if (!request.dryRun)
        return RecordingActionExecutionResult::failed(
            request.type, request.recordingId, request.backendId,
            "recording action preflight must be read-only",
            {"provider preview must be dry-run"});

    // A legacy adapter can dispatch an actual request even for dryRun.
    // Only the audited SafeMutation provider preview is safe for Public-v1.
    const auto adapter = backendExecutorAdapterRegistry_.findAdapter(
        request.backendId);
    const auto* safe = adapter.found && adapter.adapter
        ? dynamic_cast<const RestfulApiRecordingActionBackendExecutorAdapter*>(
            adapter.adapter.get())
        : nullptr;
    if (!safe || safe->config().apiMode !=
            RestfulApiRecordingActionApiMode::SafeMutation)
        return RecordingActionExecutionResult::failed(
            request.type, request.recordingId, request.backendId,
            "provider preflight unavailable",
            {"no read-only provider preview is registered"});

    const RecordingActionRequest resolved = resolveBackendNativeId(request);
    if (backendRegistry_ != nullptr)
    {
        const auto lookup = backendPolicyProvider_.policyForBackend(
            *backendRegistry_, resolved.backendId);
        return executionService_.execute(
            resolved, backendExecutorAdapterRegistry_, lookup.policy);
    }
    return executionService_.execute(
        resolved, backendExecutorAdapterRegistry_);
}

RecordingActionExecutionResult RecordingActionExecutionController::executeResult(
    const RecordingActionRequest& request)
{
    const RecordingActionRequest resolvedRequest =
        resolveBackendNativeId(request);

    RecordingActionExecutionResult result;
    if (backendRegistry_ != nullptr)
    {
        const auto lookup = backendPolicyProvider_.policyForBackend(
            *backendRegistry_, resolvedRequest.backendId);
        result = executionService_.execute(
            resolvedRequest, backendExecutorAdapterRegistry_, lookup.policy);
    }
    else
    {
        result = executionService_.execute(
            resolvedRequest, backendExecutorAdapterRegistry_);
    }

    const bool readbackVerified = refreshAfterSuccessfulExecution(
        result, resolvedRequest);
    return enrichExecutionResult(
        std::move(result), resolvedRequest, readbackVerified);
}

ApiResponse RecordingActionExecutionController::execute(
    const RecordingActionRequest& request)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = jsonSerializer_.serialize(executeResult(request));
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
