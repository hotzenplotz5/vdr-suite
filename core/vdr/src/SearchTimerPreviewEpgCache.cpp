#include "SearchTimerPreviewEpgCache.h"

SearchTimerPreviewEpgCacheEntry::SearchTimerPreviewEpgCacheEntry()
    : status(SearchTimerPreviewEpgCacheStatus::Unknown)
{
}

SearchTimerPreviewEpgCache::SearchTimerPreviewEpgCache()
{
}

SearchTimerPreviewEpgCacheStatus SearchTimerPreviewEpgCache::statusForBackend(
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

bool SearchTimerPreviewEpgCache::isReadyForBackend(
    const std::string& backendId) const
{
    return statusForBackend(backendId) ==
        SearchTimerPreviewEpgCacheStatus::Ready;
}

const std::vector<VdrEvent>* SearchTimerPreviewEpgCache::eventsForBackend(
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

std::size_t SearchTimerPreviewEpgCache::readyEventCountForBackend(
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

void SearchTimerPreviewEpgCache::markWarming(
    const std::string& backendId)
{
    SearchTimerPreviewEpgCacheEntry& entry =
        entryForBackend(backendId);

    entry.status = SearchTimerPreviewEpgCacheStatus::Warming;
    entry.events.clear();
}

void SearchTimerPreviewEpgCache::updateReady(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    SearchTimerPreviewEpgCacheEntry& entry =
        entryForBackend(backendId);

    entry.status = SearchTimerPreviewEpgCacheStatus::Ready;
    entry.events = events;
}

void SearchTimerPreviewEpgCache::markStale(
    const std::string& backendId)
{
    SearchTimerPreviewEpgCacheEntry& entry =
        entryForBackend(backendId);

    entry.status = SearchTimerPreviewEpgCacheStatus::Stale;
}

void SearchTimerPreviewEpgCache::markUnavailable(
    const std::string& backendId)
{
    SearchTimerPreviewEpgCacheEntry& entry =
        entryForBackend(backendId);

    entry.status = SearchTimerPreviewEpgCacheStatus::Unavailable;
    entry.events.clear();
}

void SearchTimerPreviewEpgCache::clearBackend(
    const std::string& backendId)
{
    entriesByBackend_.erase(normalizeBackendId(backendId));
}

void SearchTimerPreviewEpgCache::clear()
{
    entriesByBackend_.clear();
}

std::vector<std::string> SearchTimerPreviewEpgCache::backendIds() const
{
    std::vector<std::string> ids;

    for (const auto& entry : entriesByBackend_)
    {
        ids.push_back(entry.first);
    }

    return ids;
}

std::string SearchTimerPreviewEpgCache::normalizeBackendId(
    const std::string& backendId)
{
    if (backendId.empty())
    {
        return "default";
    }

    return backendId;
}

SearchTimerPreviewEpgCacheEntry& SearchTimerPreviewEpgCache::entryForBackend(
    const std::string& backendId)
{
    return entriesByBackend_[normalizeBackendId(backendId)];
}

const SearchTimerPreviewEpgCacheEntry* SearchTimerPreviewEpgCache::findEntryForBackend(
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
