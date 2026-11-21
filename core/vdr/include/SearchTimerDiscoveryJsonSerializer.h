#pragma once

#include "SearchTimerDiscovery.h"

#include <string>

class SearchTimerDiscoveryJsonSerializer {
public:
    std::string serialize(
        const SearchTimerDiscoveryCatalog& catalog) const;
};
