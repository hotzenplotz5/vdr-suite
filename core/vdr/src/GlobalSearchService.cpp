#include "GlobalSearchService.h"

#include "GlobalSearchRepository.h"

GlobalSearchService::GlobalSearchService(GlobalSearchRepository& repository)
    : repository_(repository)
{
}

GlobalSearchResult GlobalSearchService::search(
    const std::string& backendId,
    const std::string& query,
    std::int64_t epgFrom,
    std::int64_t epgUntil,
    int limit,
    int offset) const
{
    return repository_.search(
        backendId,
        query,
        epgFrom,
        epgUntil,
        limit,
        offset);
}
