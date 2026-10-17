#pragma once

#include "VdrEvent.h"

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/*
 * Header-only by design: legacy single-target Makefile tests compile
 * SnapshotCacheService.cpp without the full VDR source list.
 *
 * Runtime refresh and Public-v1 preview can run on different threads.
 * Readers of live cache data must retain a shared immutable snapshot,
 * never a pointer into a vector being replaced by a refresh.
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
    std::shared_ptr<const std::vector<VdrEvent>> events;
};

class SearchTimerPreviewEpgCache
{
public:
    SearchTimerPreviewEpgCache() = default;

    SearchTimerPreviewEpgCacheStatus statusForBackend(
        const std::string& backendId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto* entry = findEntryForBackend(backendId);
        return entry ? entry->status :
            SearchTimerPreviewEpgCacheStatus::Unknown;
    }

    bool isReadyForBackend(const std::string& backendId) const
    {
        return statusForBackend(backendId) ==
            SearchTimerPreviewEpgCacheStatus::Ready;
    }

    // Thread-safe, lifetime-stable runtime read. A later refresh or
    // invalidation cannot change the contents of an already admitted preview.
    std::shared_ptr<const std::vector<VdrEvent>> sharedEventsForBackend(
        const std::string& backendId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto* entry = findEntryForBackend(backendId);
        return entry && entry->status == SearchTimerPreviewEpgCacheStatus::Ready
            ? entry->events : nullptr;
    }

    // Compatibility for existing single-threaded callers and tests.
    // Concurrent runtime consumers must use sharedEventsForBackend().
    const std::vector<VdrEvent>* eventsForBackend(
        const std::string& backendId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto* entry = findEntryForBackend(backendId);
        return entry && entry->status == SearchTimerPreviewEpgCacheStatus::Ready
            && entry->events ? entry->events.get() : nullptr;
    }

    std::size_t readyEventCountForBackend(
        const std::string& backendId) const
    {
        const auto snapshot = sharedEventsForBackend(backendId);
        return snapshot ? snapshot->size() : 0;
    }

    void markWarming(const std::string& backendId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = entryForBackend(backendId);
        entry.status = SearchTimerPreviewEpgCacheStatus::Warming;
        entry.events.reset();
    }

    void updateReady(
        const std::string& backendId,
        const std::vector<VdrEvent>& events)
    {
        auto immutable = std::make_shared<const std::vector<VdrEvent>>(events);
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = entryForBackend(backendId);
        entry.status = SearchTimerPreviewEpgCacheStatus::Ready;
        entry.events = std::move(immutable);
    }

    void markStale(const std::string& backendId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        entryForBackend(backendId).status =
            SearchTimerPreviewEpgCacheStatus::Stale;
    }

    void markUnavailable(const std::string& backendId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = entryForBackend(backendId);
        entry.status = SearchTimerPreviewEpgCacheStatus::Unavailable;
        entry.events.reset();
    }

    void clearBackend(const std::string& backendId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        entriesByBackend_.erase(normalizeBackendId(backendId));
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        entriesByBackend_.clear();
    }

    std::vector<std::string> backendIds() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> ids;
        for (const auto& entry : entriesByBackend_)
            ids.push_back(entry.first);
        return ids;
    }

private:
    static std::string normalizeBackendId(const std::string& backendId)
    {
        return backendId.empty() ? "default" : backendId;
    }

    SearchTimerPreviewEpgCacheEntry& entryForBackend(
        const std::string& backendId)
    {
        return entriesByBackend_[normalizeBackendId(backendId)];
    }

    const SearchTimerPreviewEpgCacheEntry* findEntryForBackend(
        const std::string& backendId) const
    {
        const auto it = entriesByBackend_.find(normalizeBackendId(backendId));
        return it == entriesByBackend_.end() ? nullptr : &it->second;
    }

    mutable std::mutex mutex_;
    std::map<std::string, SearchTimerPreviewEpgCacheEntry> entriesByBackend_;
};
