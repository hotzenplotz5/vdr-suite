#pragma once

#include "VdrEvent.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

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
    SearchTimerPreviewEpgCacheEntry();

    SearchTimerPreviewEpgCacheStatus status;
    std::vector<VdrEvent> events;
};

class SearchTimerPreviewEpgCache
{
public:
    SearchTimerPreviewEpgCache();

    SearchTimerPreviewEpgCacheStatus statusForBackend(
        const std::string& backendId) const;

    bool isReadyForBackend(
        const std::string& backendId) const;

    const std::vector<VdrEvent>* eventsForBackend(
        const std::string& backendId) const;

    std::size_t readyEventCountForBackend(
        const std::string& backendId) const;

    void markWarming(
        const std::string& backendId);

    void updateReady(
        const std::string& backendId,
        const std::vector<VdrEvent>& events);

    void markStale(
        const std::string& backendId);

    void markUnavailable(
        const std::string& backendId);

    void clearBackend(
        const std::string& backendId);

    void clear();

    std::vector<std::string> backendIds() const;

private:
    static std::string normalizeBackendId(
        const std::string& backendId);

    SearchTimerPreviewEpgCacheEntry& entryForBackend(
        const std::string& backendId);

    const SearchTimerPreviewEpgCacheEntry* findEntryForBackend(
        const std::string& backendId) const;

    std::map<std::string, SearchTimerPreviewEpgCacheEntry> entriesByBackend_;
};
