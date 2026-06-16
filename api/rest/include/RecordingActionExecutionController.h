#pragma once

#include "BackendRegistry.h"
#include "DashboardController.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionBackendPolicyProvider.h"
#include "RecordingActionRequest.h"
#include "RecordingActionSafetyResultJsonSerializer.h"
#include "RecordingActionSafetyService.h"

#include <string>

class RecordingActionExecutionResultJsonSerializer;
class RecordingActionExecutionService;
class RecordingActionValidationRequestParser;

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

    ApiResponse execute(
        const RecordingActionRequest& request);

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
};
