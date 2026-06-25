#include "RestfulApiSearchTimerDiscoveryProvider.h"

#include <utility>

RestfulApiSearchTimerDiscoveryProvider::RestfulApiSearchTimerDiscoveryProvider(
    std::string configuredBackendId,
    std::string discoveryEndpoint)
    : configuredBackendId_(std::move(configuredBackendId)),
      discoveryEndpoint_(std::move(discoveryEndpoint))
{
}

const std::string& RestfulApiSearchTimerDiscoveryProvider::configuredBackendId() const
{
    return configuredBackendId_;
}

const std::string& RestfulApiSearchTimerDiscoveryProvider::discoveryEndpoint() const
{
    return discoveryEndpoint_;
}

SearchTimerDiscoveryCatalog RestfulApiSearchTimerDiscoveryProvider::discover(
    const std::string& backendId) const
{
    SearchTimerDiscoveryCatalog catalog;

    catalog.setBackendId(
        backendId.empty()
            ? configuredBackendId_
            : backendId);

    return catalog;
}
