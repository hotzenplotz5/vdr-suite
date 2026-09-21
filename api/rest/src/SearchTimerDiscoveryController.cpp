#include "SearchTimerDiscoveryController.h"

#include "SearchTimerDiscoveryJsonSerializer.h"
#include "SearchTimerDiscoveryService.h"

SearchTimerDiscoveryController::SearchTimerDiscoveryController(
    SearchTimerDiscoveryJsonSerializer& jsonSerializer)
    : discoveryService_(nullptr),
      jsonSerializer_(jsonSerializer)
{
}

SearchTimerDiscoveryController::SearchTimerDiscoveryController(
    SearchTimerDiscoveryService& discoveryService,
    SearchTimerDiscoveryJsonSerializer& jsonSerializer)
    : discoveryService_(&discoveryService),
      jsonSerializer_(jsonSerializer)
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

ApiResponse SearchTimerDiscoveryController::getDiscovery(
    const std::string& backendId)
{
    if (discoveryService_ == nullptr)
    {
        ApiResponse response;
        response.statusCode = 503;
        response.contentType = "application/json";
        response.body = "{\"error\":\"SearchTimer discovery service not configured\"}";
        return response;
    }

    return getDiscovery(
        discoveryService_->discover(backendId));
}
