#pragma once

#include "SearchTimerDiscovery.h"

#include <string>

class ISearchTimerDiscoveryProvider;

class SearchTimerDiscoveryService {
public:
    explicit SearchTimerDiscoveryService(
        ISearchTimerDiscoveryProvider& provider);

    SearchTimerDiscoveryCatalog discover(
        const std::string& backendId) const;

private:
    ISearchTimerDiscoveryProvider& provider_;
};
