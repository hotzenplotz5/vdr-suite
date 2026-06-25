#include "SearchTimerDiscoveryController.h"

#include "SearchTimerDiscoveryJsonSerializer.h"

SearchTimerDiscoveryController::SearchTimerDiscoveryController(
    SearchTimerDiscoveryJsonSerializer& jsonSerializer)
    : jsonSerializer_(jsonSerializer)
{
}

ApiResponse SearchTimerDiscoveryController::getDiscovery(
    const SearchTimerDiscoveryCatalog& catalog)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = jsonSerializer_.serialize(catalog);

    return response;
}
