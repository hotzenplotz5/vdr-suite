#pragma once

#include "ISearchTimerDiscoveryProvider.h"

#include <string>

class RestfulApiSearchTimerDiscoveryProvider final
    : public ISearchTimerDiscoveryProvider
{
public:
    explicit RestfulApiSearchTimerDiscoveryProvider(
        std::string configuredBackendId,
        std::string discoveryEndpoint = "/searchtimers/discovery.json");

    const std::string& configuredBackendId() const;
    const std::string& discoveryEndpoint() const;

    SearchTimerDiscoveryCatalog discover(
        const std::string& backendId) const override;

private:
    std::string configuredBackendId_;
    std::string discoveryEndpoint_;
};
