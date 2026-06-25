#pragma once

#include "SearchTimerDiscovery.h"

#include <string>

class ISearchTimerDiscoveryProvider {
public:
    virtual ~ISearchTimerDiscoveryProvider() = default;

    virtual SearchTimerDiscoveryCatalog discover(
        const std::string& backendId) const = 0;
};
