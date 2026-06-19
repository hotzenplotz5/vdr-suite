#pragma once

#include "BackendRegistry.h"
#include "DashboardController.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionBackendPolicyProvider.h"
#include "RecordingActionRequest.h"
#include "RecordingActionSafetyResultJsonSerializer.h"
#include "RecordingActionSafetyService.h"

#include <functional>
#include <string>

struct RecordingActionExecutionResult;

class RecordingActionExecutionResultJsonSerializer;
class RecordingActionExecutionService;
class RecordingActionValidationRequestParser;
class VdrSnapshotReadService;

class RecordingActionExecutionController
{
public:
    RecordingActionExecutionController(
        RecordingActionExecutionService& executionService,
        RecordingActionExecutionResultJsonSerializer& jsonSerializer,
        RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry);

    RecordingActionExecutionController(
        RecordingActionExecutionService& executionService,
        RecordingActionExecutionResultJsonSerializer& jsonSerializer,
        RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry,
        RecordingActionValidationRequestParser& requestParser);

    RecordingActionExecutionController(
        RecordingActionExecutionService& executionService,
        RecordingActionExecutionResultJsonSerializer& jsonSerializer,
        RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry,
        BackendRegistry& backendRegistry,
        RecordingActionValidationRequestParser& requestParser);

    RecordingActionExecutionController(
        RecordingActionExecutionService& executionService,
        RecordingActionExecutionResultJsonSerializer& jsonSerializer,
        RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry,
        BackendRegistry& backendRegistry,
        RecordingActionValidationRequestParser& requestParser,
        VdrSnapshotReadService& snapshotReadService);

    ApiResponse execute(
        const RecordingActionRequest& request);

    void setAfterSuccessfulExecutionCallback(
        std::function<void()> callback);

    RecordingActionRequest resolveBackendNativeId(
        const RecordingActionRequest& request) const;

    ApiResponse safety(
        const RecordingActionRequest& request,
        const RecordingActionSafetyContext& context);

    ApiResponse executeBody(
        const std::string& body);

private:
    RecordingActionExecutionService& executionService_;
    RecordingActionExecutionResultJsonSerializer& jsonSerializer_;
    RecordingActionSafetyResultJsonSerializer safetyJsonSerializer_;
    RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry_;
    RecordingActionBackendPolicyProvider backendPolicyProvider_;
    BackendRegistry* backendRegistry_;
    RecordingActionValidationRequestParser* requestParser_;
    VdrSnapshotReadService* snapshotReadService_;
    std::function<void()> afterSuccessfulExecution_;

    bool refreshAfterSuccessfulExecution(
        const RecordingActionExecutionResult& result) const;

    RecordingActionExecutionResult enrichExecutionResult(
        RecordingActionExecutionResult result,
        const RecordingActionRequest& resolvedRequest,
        bool snapshotRefreshed) const;
};
