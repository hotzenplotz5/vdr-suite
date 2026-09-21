#pragma once

#include "SearchTimerPreviewEpgCache.h"

#include <string>

class SearchTimerPreviewEpgCacheStaleGuard
{
public:
    static bool markBackendStaleWhenReady(
        SearchTimerPreviewEpgCache& cache,
        const std::string& backendId)
    {
        if (!cache.isReadyForBackend(backendId))
        {
            return false;
        }

        cache.markStale(backendId);
        return true;
    }
};
