#pragma once

#include "SearchTimerPreviewEpgCache.h"
#include "VdrEventQuery.h"

#include <cstddef>
#include <functional>
#include <string>

class VdrSnapshotBuilder;
class VdrService;

struct SearchTimerPreviewEpgCacheRefreshRequest
{
    std::string backendId = "default";
    int from = -1;
    int timespan = 0;
    int start = -1;
    int limit = 0;
    int channelEventLimit = 50;

    VdrEventQuery toEventQuery() const
    {
        VdrEventQuery query;
        query.from = from;
        query.timespan = timespan;
        query.start = start;
        query.limit = limit;
        query.channelEventLimit = channelEventLimit;
        return query;
    }
};

struct SearchTimerPreviewEpgCacheRefreshResult
{
    std::string backendId;
    std::string status;
    bool available = false;
    std::size_t eventCount = 0;
};

class SearchTimerPreviewEpgCacheRefreshService
{
public:
    SearchTimerPreviewEpgCacheRefreshService(
        SearchTimerPreviewEpgCache& cache,
        VdrSnapshotBuilder& snapshotBuilder);
    SearchTimerPreviewEpgCacheRefreshService(
        SearchTimerPreviewEpgCache& cache,
        VdrService& epgService);

    SearchTimerPreviewEpgCacheRefreshResult refreshBackend(
        const SearchTimerPreviewEpgCacheRefreshRequest& request);

private:
    static std::string normalizeBackendId(const std::string& backendId);

    SearchTimerPreviewEpgCache& cache_;
    std::function<std::vector<VdrEvent>(const VdrEventQuery&)> loadEvents_;
};
