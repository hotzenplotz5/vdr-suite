#pragma once

#include "DashboardController.h"
#include "SearchTimerDiscovery.h"

class SearchTimerDiscoveryJsonSerializer;

class SearchTimerDiscoveryController {
public:
    explicit SearchTimerDiscoveryController(
        SearchTimerDiscoveryJsonSerializer& jsonSerializer);

    ApiResponse getDiscovery(
        const SearchTimerDiscoveryCatalog& catalog);

private:
    SearchTimerDiscoveryJsonSerializer& jsonSerializer_;
};
