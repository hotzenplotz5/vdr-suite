#include "SearchTimerPreviewEpgCacheRefreshService.h"

#include "SearchTimerPreviewEpgCacheChangeInvalidator.h"
#include "VdrSnapshotBuilder.h"

#include <string>
#include <vector>

SearchTimerPreviewEpgCacheRefreshService::SearchTimerPreviewEpgCacheRefreshService(
    SearchTimerPreviewEpgCache& cache,
    VdrSnapshotBuilder& snapshotBuilder)
    : cache_(cache),
      snapshotBuilder_(snapshotBuilder)
{
    SearchTimerPreviewEpgCacheChangeInvalidator::registerRuntimeCache(cache_);
}

SearchTimerPreviewEpgCacheRefreshResult
SearchTimerPreviewEpgCacheRefreshService::refreshBackend(
    const SearchTimerPreviewEpgCacheRefreshRequest& request)
{
    const std::string backendId = normalizeBackendId(request.backendId);

    SearchTimerPreviewEpgCacheRefreshResult result;
    result.backendId = backendId;
    result.status = "warming";
    result.available = false;
    result.eventCount = 0;

    cache_.markWarming(backendId);

    try {
        const std::vector<VdrEvent> events =
            snapshotBuilder_.buildEvents(request.toEventQuery());

        cache_.updateReady(backendId, events);

        result.status = "ready";
        result.available = true;
        result.eventCount = events.size();
    } catch (...) {
        cache_.markUnavailable(backendId);

        result.status = "unavailable";
        result.available = false;
        result.eventCount = 0;
    }

    return result;
}

std::string SearchTimerPreviewEpgCacheRefreshService::normalizeBackendId(
    const std::string& backendId)
{
    if (backendId.empty()) {
        return "default";
    }

    return backendId;
}
