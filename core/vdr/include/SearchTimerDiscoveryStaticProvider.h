#pragma once

#include "ISearchTimerDiscoveryProvider.h"

class SearchTimerDiscoveryStaticProvider final
    : public ISearchTimerDiscoveryProvider
{
public:
    SearchTimerDiscoveryCatalog discover(
        const std::string& backendId) const override;
};
