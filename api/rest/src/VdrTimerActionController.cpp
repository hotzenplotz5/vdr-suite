#include "VdrTimerActionController.h"

#include "IVdrTimerActionExecutor.h"
#include "VdrTimerActionRequestParser.h"
#include "VdrTimerActionResultJsonSerializer.h"
#include "VdrTimerActionService.h"

VdrTimerActionController::VdrTimerActionController(
    VdrTimerActionService& actionService,
    VdrTimerActionResultJsonSerializer& jsonSerializer)
    : actionService_(actionService),
      jsonSerializer_(jsonSerializer),
      requestParser_(nullptr)
{
}

VdrTimerActionController::VdrTimerActionController(
    VdrTimerActionService& actionService,
    VdrTimerActionResultJsonSerializer& jsonSerializer,
    VdrTimerActionRequestParser& requestParser)
    : actionService_(actionService),
      jsonSerializer_(jsonSerializer),
      requestParser_(&requestParser)
{
}

ApiResponse VdrTimerActionController::create(
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor)
{
    return execute(
        VdrTimerActionType::Create,
        request,
        executor);
}

ApiResponse VdrTimerActionController::update(
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor)
{
    return execute(
        VdrTimerActionType::Update,
        request,
        executor);
}

ApiResponse VdrTimerActionController::remove(
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor)
{
    return execute(
        VdrTimerActionType::Delete,
        request,
        executor);
}

ApiResponse VdrTimerActionController::createBody(
    const std::string& body,
    IVdrTimerActionExecutor& executor)
{
    if (requestParser_ == nullptr)
    {
        return parserUnavailableResponse();
    }

    return create(
        requestParser_->parse(body),
        executor);
}

ApiResponse VdrTimerActionController::updateBody(
    const std::string& body,
    IVdrTimerActionExecutor& executor)
{
    if (requestParser_ == nullptr)
    {
        return parserUnavailableResponse();
    }

    return update(
        requestParser_->parse(body),
        executor);
}

ApiResponse VdrTimerActionController::removeBody(
    const std::string& body,
    IVdrTimerActionExecutor& executor)
{
    if (requestParser_ == nullptr)
    {
        return parserUnavailableResponse();
    }

    return remove(
        requestParser_->parse(body),
        executor);
}

ApiResponse VdrTimerActionController::execute(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request,
    IVdrTimerActionExecutor& executor)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    if (type == VdrTimerActionType::Create)
    {
        response.body =
            jsonSerializer_.serialize(
                actionService_.create(
                    request,
                    executor));

        return response;
    }

    if (type == VdrTimerActionType::Update)
    {
        response.body =
            jsonSerializer_.serialize(
                actionService_.update(
                    request,
                    executor));

        return response;
    }

    if (type == VdrTimerActionType::Delete)
    {
        response.body =
            jsonSerializer_.serialize(
                actionService_.remove(
                    request,
                    executor));

        return response;
    }

    response.body =
        jsonSerializer_.serialize(
            actionService_.toggle(
                request,
                executor));

    return response;
}

ApiResponse VdrTimerActionController::parserUnavailableResponse() const
{
    ApiResponse response;

    response.statusCode = 500;
    response.contentType = "application/json";
    response.body = "{\"error\":\"vdr timer action request parser unavailable\"}";

    return response;
}
