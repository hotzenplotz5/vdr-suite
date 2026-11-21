#pragma once

#include "DashboardController.h"
#include "SearchTimerDiscovery.h"

#include <string>

class SearchTimerDiscoveryJsonSerializer;
class SearchTimerDiscoveryService;

class SearchTimerDiscoveryController {
public:
    explicit SearchTimerDiscoveryController(
        SearchTimerDiscoveryJsonSerializer& jsonSerializer);

    SearchTimerDiscoveryController(
        SearchTimerDiscoveryService& discoveryService,
        SearchTimerDiscoveryJsonSerializer& jsonSerializer);

    ApiResponse getDiscovery(
        const SearchTimerDiscoveryCatalog& catalog);

    ApiResponse getDiscovery(
        const std::string& backendId);

private:
    SearchTimerDiscoveryService* discoveryService_;
    SearchTimerDiscoveryJsonSerializer& jsonSerializer_;
};
