#pragma once

#include "VdrEvent.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

/*
 * Header-only by design: many legacy single-target Makefile tests compile
 * SnapshotCacheService.cpp without linking the full VDR source list. Keeping
 * this small cache inline prevents fragile per-target linker dependencies.
 */
enum class SearchTimerPreviewEpgCacheStatus
{
    Unknown,
    Warming,
    Ready,
    Stale,
    Unavailable
};

struct SearchTimerPreviewEpgCacheEntry
{
    SearchTimerPreviewEpgCacheEntry()
        : status(SearchTimerPreviewEpgCacheStatus::Unknown)
    {
    }

    SearchTimerPreviewEpgCacheStatus status;
    std::vector<VdrEvent> events;
};

class SearchTimerPreviewEpgCache
{
public:
    SearchTimerPreviewEpgCache()
    {
    }

    SearchTimerPreviewEpgCacheStatus statusForBackend(
        const std::string& backendId) const
    {
        const SearchTimerPreviewEpgCacheEntry* entry =
            findEntryForBackend(backendId);

        if (entry == nullptr)
        {
            return SearchTimerPreviewEpgCacheStatus::Unknown;
        }

        return entry->status;
    }

    bool isReadyForBackend(
        const std::string& backendId) const
    {
        return statusForBackend(backendId) ==
            SearchTimerPreviewEpgCacheStatus::Ready;
    }

    const std::vector<VdrEvent>* eventsForBackend(
        const std::string& backendId) const
    {
        const SearchTimerPreviewEpgCacheEntry* entry =
            findEntryForBackend(backendId);

        if (entry == nullptr ||
            entry->status != SearchTimerPreviewEpgCacheStatus::Ready)
        {
            return nullptr;
        }

        return &entry->events;
    }

    std::size_t readyEventCountForBackend(
        const std::string& backendId) const
    {
        const std::vector<VdrEvent>* events =
            eventsForBackend(backendId);

        if (events == nullptr)
        {
            return 0;
        }

        return events->size();
    }

    void markWarming(
        const std::string& backendId)
    {
        SearchTimerPreviewEpgCacheEntry& entry =
            entryForBackend(backendId);

        entry.status = SearchTimerPreviewEpgCacheStatus::Warming;
        entry.events.clear();
    }

    void updateReady(
        const std::string& backendId,
        const std::vector<VdrEvent>& events)
    {
        SearchTimerPreviewEpgCacheEntry& entry =
            entryForBackend(backendId);

        entry.status = SearchTimerPreviewEpgCacheStatus::Ready;
        entry.events = events;
    }

    void markStale(
        const std::string& backendId)
    {
        SearchTimerPreviewEpgCacheEntry& entry =
            entryForBackend(backendId);

        entry.status = SearchTimerPreviewEpgCacheStatus::Stale;
    }

    void markUnavailable(
        const std::string& backendId)
    {
        SearchTimerPreviewEpgCacheEntry& entry =
            entryForBackend(backendId);

        entry.status = SearchTimerPreviewEpgCacheStatus::Unavailable;
        entry.events.clear();
    }

    void clearBackend(
        const std::string& backendId)
    {
        entriesByBackend_.erase(normalizeBackendId(backendId));
    }

    void clear()
    {
        entriesByBackend_.clear();
    }

    std::vector<std::string> backendIds() const
    {
        std::vector<std::string> ids;

        for (const auto& entry : entriesByBackend_)
        {
            ids.push_back(entry.first);
        }

        return ids;
    }

private:
    static std::string normalizeBackendId(
        const std::string& backendId)
    {
        if (backendId.empty())
        {
            return "default";
        }

        return backendId;
    }

    SearchTimerPreviewEpgCacheEntry& entryForBackend(
        const std::string& backendId)
    {
        return entriesByBackend_[normalizeBackendId(backendId)];
    }

    const SearchTimerPreviewEpgCacheEntry* findEntryForBackend(
        const std::string& backendId) const
    {
        const auto it =
            entriesByBackend_.find(normalizeBackendId(backendId));

        if (it == entriesByBackend_.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    std::map<std::string, SearchTimerPreviewEpgCacheEntry> entriesByBackend_;
};
