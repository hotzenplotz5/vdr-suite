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

    static void registerRuntimeCache(
        SearchTimerPreviewEpgCache& cache)
    {
        runtimeCache() = &cache;
    }

    static void clearRuntimeCache()
    {
        runtimeCache() = nullptr;
    }

    static bool invalidateRegisteredRuntimeCacheForChangeEvents(
        const std::string& backendId,
        const std::vector<VdrChangeEvent>& changeEvents)
    {
        SearchTimerPreviewEpgCache* cache = runtimeCache();

        if (cache == nullptr)
        {
            return false;
        }

        return invalidateForChangeEvents(
            *cache,
            backendId,
            changeEvents);
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

    static SearchTimerPreviewEpgCache*& runtimeCache()
    {
        static SearchTimerPreviewEpgCache* cache = nullptr;
        return cache;
    }
};
