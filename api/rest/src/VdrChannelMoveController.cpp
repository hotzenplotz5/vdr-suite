
#include "VdrChannelMoveController.h"

VdrChannelMoveController::VdrChannelMoveController(
    VdrChannelMoveExecutionService& executionService,
    VdrChannelMoveResultJsonSerializer& jsonSerializer,
    VdrChannelMoveRequestParser& requestParser,
    const VdrChannelMoveExecutorAdapterRegistry& registry,
    const BackendRegistryService& backendRegistryService,
    const BackendAccessPolicy& backendAccessPolicy)
    : executionService_(executionService),
      jsonSerializer_(jsonSerializer),
      requestParser_(requestParser),
      registry_(registry),
      backendRegistryService_(backendRegistryService),
      backendAccessPolicy_(backendAccessPolicy)
{
}

ApiResponse VdrChannelMoveController::moveBody(
    const std::string& body)
{
    const VdrChannelMoveRequest request =
        requestParser_.parse(body);

    const BackendAccessDecision accessDecision =
        backendAccessPolicy_.canWriteToBackend(
            backendRegistryService_,
            request.backendId);

    const VdrChannelMoveResult result =
        executionService_.execute(
            request,
            registry_,
            accessDecision);

    if (result.success &&
        !result.dryRun &&
        afterSuccessfulMoveCallback_)
    {
        try
        {
            afterSuccessfulMoveCallback_(result.backendId);
        }
        catch (...)
        {
        }
    }

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(result);

    return response;
}

void VdrChannelMoveController::setAfterSuccessfulMoveCallback(
    AfterSuccessfulMoveCallback callback)
{
    afterSuccessfulMoveCallback_ = callback;
}
