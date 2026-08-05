#pragma once

#include "DashboardController.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class BackendRegistryService;
class GlobalSearchService;

struct GlobalSearchPersonPortrait
{
    std::string name;
    std::string role;
    std::string backendNativeId;
    int index = -1;
    int assignmentRevision = 0;
};

class GlobalSearchController
{
public:
    using PersonPortraitLookup = std::function<
        std::vector<GlobalSearchPersonPortrait>(const std::string&)>;

    GlobalSearchController(
        GlobalSearchService& service,
        BackendRegistryService& backendRegistryService,
        PersonPortraitLookup personPortraitLookup = {});

    void setPersonPortraitLookup(
        PersonPortraitLookup personPortraitLookup);

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
    PersonPortraitLookup personPortraitLookup_;
};