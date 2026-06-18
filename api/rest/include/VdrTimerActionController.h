#pragma once

#include "DashboardController.h"
#include "VdrTimerOperationRequest.h"
#include "VdrTimerActionResult.h"

#include <string>

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

private:
    ApiResponse execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request,
        IVdrTimerActionExecutor& executor);

    ApiResponse parserUnavailableResponse() const;

    VdrTimerActionService& actionService_;
    VdrTimerActionResultJsonSerializer& jsonSerializer_;
    VdrTimerActionRequestParser* requestParser_;
};
