#pragma once

#include "SearchTimerPreviewEpgCache.h"
#include "VdrChangeEvent.h"

#include <string>
#include <vector>

class SearchTimerPreviewEpgCacheInvalidationService
{
public:
    explicit SearchTimerPreviewEpgCacheInvalidationService(
        SearchTimerPreviewEpgCache& cache)
        : cache_(cache)
    {
    }

    bool invalidateForChanges(
        const std::string& backendId,
        const std::vector<VdrChangeEvent>& changeEvents)
    {
        if (!containsEventsChanged(changeEvents)) {
            return false;
        }

        const SearchTimerPreviewEpgCacheStatus currentStatus =
            cache_.statusForBackend(backendId);

        if (currentStatus != SearchTimerPreviewEpgCacheStatus::Ready) {
            return false;
        }

        cache_.markStale(backendId);
        return true;
    }

private:
    static bool containsEventsChanged(
        const std::vector<VdrChangeEvent>& changeEvents)
    {
        for (const VdrChangeEvent& changeEvent : changeEvents) {
            if (changeEvent.type() == VdrChangeType::EventsChanged) {
                return true;
            }
        }

        return false;
    }

    SearchTimerPreviewEpgCache& cache_;
};
