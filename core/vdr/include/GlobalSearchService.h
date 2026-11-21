#pragma once

#include "GlobalSearchResult.h"

#include <cstdint>
#include <string>

class GlobalSearchRepository;

class GlobalSearchService
{
public:
    explicit GlobalSearchService(GlobalSearchRepository& repository);

    GlobalSearchResult search(
        const std::string& backendId,
        const std::string& query,
        std::int64_t epgFrom,
        std::int64_t epgUntil,
        int limit,
        int offset) const;

private:
    GlobalSearchRepository& repository_;
};
