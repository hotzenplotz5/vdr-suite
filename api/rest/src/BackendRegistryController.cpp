#include "BackendRegistryController.h"

#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryService.h"

BackendRegistryController::BackendRegistryController(
    BackendRegistryService& registryService,
    BackendRegistryJsonSerializer& jsonSerializer)
    : registryService_(registryService),
      jsonSerializer_(jsonSerializer)
{
}

ApiResponse BackendRegistryController::getBackends()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serializeBackends(
            registryService_.listBackends());

    return response;
}

ApiResponse BackendRegistryController::getDefaultBackend()
{
    ApiResponse response;

    const auto backend =
        registryService_.defaultBackend();

    response.statusCode = 200;
    response.contentType = "application/json";

    if (backend.has_value())
    {
        response.body =
            jsonSerializer_.serializeBackend(
                *backend);
    }
    else
    {
        response.body = "{}";
    }

    return response;
}
