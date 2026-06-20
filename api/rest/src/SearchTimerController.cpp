#include "SearchTimerController.h"

#include "SearchTimerResult.h"
#include "SearchTimerResultJsonSerializer.h"

SearchTimerController::SearchTimerController(
    SearchTimerResultJsonSerializer& jsonSerializer)
    : jsonSerializer_(jsonSerializer)
{
}

ApiResponse SearchTimerController::getSearchTimers(
    const SearchTimerResult& result)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = jsonSerializer_.serialize(result);

    return response;
}