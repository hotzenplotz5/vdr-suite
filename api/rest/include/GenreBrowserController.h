#pragma once

#include "DashboardController.h"

#include <cstdint>
#include <string>

class BackendRegistryService;
class GenreIndexRepository;

class GenreBrowserController
{
public:
    GenreBrowserController(
        GenreIndexRepository& repository,
        BackendRegistryService& backendRegistryService);

    ApiResponse getOverview(
        const std::string& backendId,
        const std::string& scope,
        const std::string& locale,
        std::int64_t fromTime,
        std::int64_t untilTime) const;

    ApiResponse getRecordings(
        const std::string& backendId,
        const std::string& genreId,
        int limit,
        int offset) const;

    ApiResponse getEpg(
        const std::string& backendId,
        const std::string& contentClass,
        const std::string& genreId,
        std::int64_t fromTime,
        std::int64_t untilTime,
        int limit,
        int offset) const;

private:
    GenreIndexRepository& repository_;
    BackendRegistryService& backendRegistryService_;
};
