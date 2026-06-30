#pragma once

#include "DashboardController.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "VdrTimerActionExecutionService.h"
#include "VdrTimerActionResult.h"
#include "VdrTimerOperationRequest.h"

#include <string>

class BackendAccessPolicy;
class BackendRegistryService;
class IVdrTimerActionExecutor;
class VdrTimerActionRequestParser;
class VdrTimerActionResultJsonSerializer;
class VdrTimerActionService;

class VdrTimerActionController
{
public:
    VdrTimerActionController(
        VdrTimerActionService& actionService,
        VdrTimerActionResultJsonSerializer& jsonSerializer);

    VdrTimerActionController(
        VdrTimerActionService& actionService,
        VdrTimerActionResultJsonSerializer& jsonSerializer,
        VdrTimerActionRequestParser& requestParser);

    VdrTimerActionController(
        VdrTimerActionExecutionService& executionService,
        VdrTimerActionResultJsonSerializer& jsonSerializer,
        VdrTimerActionRequestParser& requestParser);

    VdrTimerActionController(
        VdrTimerActionExecutionService& executionService,
        VdrTimerActionResultJsonSerializer& jsonSerializer,
        VdrTimerActionRequestParser& requestParser,
        const BackendRegistryService& backendRegistryService,
        const BackendAccessPolicy& backendAccessPolicy);

    ApiResponse create(
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor);

    ApiResponse update(
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor);

    ApiResponse remove(
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor);

    ApiResponse createBody(
        const std::string& body,
        IVdrTimerActionExecutor& executor);

    ApiResponse updateBody(
        const std::string& body,
        IVdrTimerActionExecutor& executor);

    ApiResponse removeBody(
        const std::string& body,
        IVdrTimerActionExecutor& executor);

    ApiResponse createBody(
        const std::string& body,
        const VdrTimerActionExecutorAdapterRegistry& registry);

    ApiResponse updateBody(
        const std::string& body,
        const VdrTimerActionExecutorAdapterRegistry& registry);

    ApiResponse removeBody(
        const std::string& body,
        const VdrTimerActionExecutorAdapterRegistry& registry);

private:
    bool hasBackendAccessPolicy() const;

    ApiResponse execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor);

    ApiResponse execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request,
        const VdrTimerActionExecutorAdapterRegistry& registry);

    ApiResponse parserUnavailableResponse() const;

    VdrTimerActionService* actionService_;
    VdrTimerActionExecutionService* executionService_;
    VdrTimerActionResultJsonSerializer& jsonSerializer_;
    VdrTimerActionRequestParser* requestParser_;
    const BackendRegistryService* backendRegistryService_;
    const BackendAccessPolicy* backendAccessPolicy_;
};
