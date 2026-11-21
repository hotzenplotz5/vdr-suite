#include "ContentRatingController.h"

#include "ContentRatingResolutionJsonSerializer.h"

ContentRatingController::ContentRatingController(
    ContentRatingResolutionJsonSerializer& jsonSerializer)
    : jsonSerializer_(jsonSerializer)
{
}

ApiResponse ContentRatingController::getRatingResolution(
    const ContentRatingResolutionResult& result)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(
            result);

    return response;
}
