#pragma once

#include "DashboardController.h"

#include <cstdint>
#include <string>

class BackendRegistryService;
class GlobalSearchService;

class GlobalSearchController
{
public:
    GlobalSearchController(
        GlobalSearchService& service,
        BackendRegistryService& backendRegistryService);

    ApiResponse search(
        const std::string& backendId,
        const std::string& query,
        std::int64_t epgFrom,
        std::int64_t epgUntil,
        int limit,
        int offset) const;

private:
    GlobalSearchService& service_;
    BackendRegistryService& backendRegistryService_;
};
