#include "SearchTimerPreviewEpgCacheRefreshService.h"

#include "SearchTimerPreviewEpgCacheChangeInvalidator.h"
#include "VdrSnapshotBuilder.h"
#include "VdrService.h"

#include <string>
#include <vector>

SearchTimerPreviewEpgCacheRefreshService::SearchTimerPreviewEpgCacheRefreshService(
    SearchTimerPreviewEpgCache& cache,
    VdrSnapshotBuilder& snapshotBuilder)
    : cache_(cache),
      loadEvents_([&snapshotBuilder](const VdrEventQuery& query) {
          return snapshotBuilder.buildEvents(query);
      })
{
    SearchTimerPreviewEpgCacheChangeInvalidator::registerRuntimeCache(cache_);
}

SearchTimerPreviewEpgCacheRefreshService::SearchTimerPreviewEpgCacheRefreshService(
    SearchTimerPreviewEpgCache& cache,
    VdrService& epgService)
    : cache_(cache),
      loadEvents_([&epgService](const VdrEventQuery& query) {
          return epgService.getEvents(query);
      })
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
            loadEvents_(request.toEventQuery());

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
