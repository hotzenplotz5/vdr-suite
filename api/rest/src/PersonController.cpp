#include "PersonController.h"

#include "PersonResolutionJsonSerializer.h"

PersonController::PersonController(
    PersonResolutionJsonSerializer& jsonSerializer)
    : jsonSerializer_(jsonSerializer)
{
}

ApiResponse PersonController::getPersonResolution(
    const PersonResolutionResult& result)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(
            result);

    return response;
}
