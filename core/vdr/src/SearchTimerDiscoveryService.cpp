#include "SearchTimerDiscoveryService.h"

#include "ISearchTimerDiscoveryProvider.h"

SearchTimerDiscoveryService::SearchTimerDiscoveryService(
    ISearchTimerDiscoveryProvider& provider)
    : provider_(provider)
{
}

SearchTimerDiscoveryCatalog SearchTimerDiscoveryService::discover(
    const std::string& backendId) const
{
    return provider_.discover(backendId);
}
