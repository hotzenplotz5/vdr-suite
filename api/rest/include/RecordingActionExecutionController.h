#pragma once

#include "DashboardController.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionRequest.h"

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

    ApiResponse execute(
        const RecordingActionRequest& request);

    ApiResponse executeBody(
        const std::string& body);

private:
    RecordingActionExecutionService& executionService_;
    RecordingActionExecutionResultJsonSerializer& jsonSerializer_;
    RecordingActionBackendExecutorAdapterRegistry& backendExecutorAdapterRegistry_;
    RecordingActionValidationRequestParser* requestParser_;
};
