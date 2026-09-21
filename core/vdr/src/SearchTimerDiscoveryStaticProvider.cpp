#include "SearchTimerDiscoveryStaticProvider.h"

SearchTimerDiscoveryCatalog SearchTimerDiscoveryStaticProvider::discover(
    const std::string& backendId) const
{
    SearchTimerDiscoveryCatalog catalog;
    catalog.setBackendId(backendId);
    return catalog;
}
