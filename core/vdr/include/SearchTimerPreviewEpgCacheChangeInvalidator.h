#pragma once

#include "SearchTimerPreviewEpgCache.h"
#include "SearchTimerPreviewEpgCacheStaleGuard.h"
#include "VdrChangeEvent.h"

#include <string>
#include <vector>

class SearchTimerPreviewEpgCacheChangeInvalidator
{
public:
    static bool invalidateForChangeEvents(
        SearchTimerPreviewEpgCache& cache,
        const std::string& backendId,
        const std::vector<VdrChangeEvent>& changeEvents)
    {
        if (!hasEventDomainChange(changeEvents))
        {
            return false;
        }

        return SearchTimerPreviewEpgCacheStaleGuard::markBackendStaleWhenReady(
            cache,
            backendId);
    }

private:
    static bool hasEventDomainChange(
        const std::vector<VdrChangeEvent>& changeEvents)
    {
        for (const VdrChangeEvent& changeEvent : changeEvents)
        {
            if (changeEvent.type() == VdrChangeType::EventsChanged)
            {
                return true;
            }
        }

        return false;
    }
};
