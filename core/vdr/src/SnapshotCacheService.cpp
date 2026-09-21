#include "SnapshotCacheService.h"

#include <algorithm>

SnapshotCacheService::SnapshotCacheService(SnapshotCache& cache)
    : cache_(cache),
      generation_(0),
      searchTimerPreviewEpgCache_()
{
}

SnapshotCache& SnapshotCacheService::cache()
{
    return cache_;
}

const SnapshotCache& SnapshotCacheService::cache() const
{
    return cache_;
}

SearchTimerPreviewEpgCache& SnapshotCacheService::searchTimerPreviewEpgCache()
{
    return searchTimerPreviewEpgCache_;
}

const SearchTimerPreviewEpgCache& SnapshotCacheService::searchTimerPreviewEpgCache() const
{
    return searchTimerPreviewEpgCache_;
}

int SnapshotCacheService::generation() const
{
    return generation_;
}

std::string SnapshotCacheService::backendId() const
{
    if (!cache_.hasSnapshot())
    {
        return "default";
    }

    return cache_.snapshot().backendId;
}

void SnapshotCacheService::incrementGeneration()
{
    ++generation_;
}

VdrSnapshot SnapshotCacheService::snapshotForBackendOrEmpty(
    const std::string& backendId) const
{
    const VdrSnapshot* snapshot = cache_.snapshotForBackend(backendId);

    if (snapshot == nullptr)
    {
        VdrSnapshot emptySnapshot;
        emptySnapshot.backendId = backendId;
        return emptySnapshot;
    }

    return *snapshot;
}

void SnapshotCacheService::updatePreviewCacheFromSnapshotIfEventsPresent(
    const std::string& backendId,
    const VdrSnapshot& snapshot)
{
    if (!snapshot.events.empty())
    {
        searchTimerPreviewEpgCache_.updateReady(
            backendId,
            snapshot.events);
    }
}

void SnapshotCacheService::updateSnapshot(const VdrSnapshot& snapshot)
{
    cache_.update(snapshot);
    updatePreviewCacheFromSnapshotIfEventsPresent(
        snapshot.backendId,
        snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateStatus(const VdrStatus& status)
{
    VdrSnapshot snapshot = cache_.hasSnapshot()
        ? cache_.snapshot()
        : VdrSnapshot{};

    snapshot.status = status;
    cache_.update(snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateRecordings(const std::vector<VdrRecording>& recordings)
{
    VdrSnapshot snapshot = cache_.hasSnapshot()
        ? cache_.snapshot()
        : VdrSnapshot{};

    snapshot.recordings = recordings;
    cache_.update(snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateTimers(const std::vector<VdrTimer>& timers)
{
    VdrSnapshot snapshot = cache_.hasSnapshot()
        ? cache_.snapshot()
        : VdrSnapshot{};

    snapshot.timers = timers;
    cache_.update(snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateChannels(const std::vector<VdrChannel>& channels)
{
    VdrSnapshot snapshot = cache_.hasSnapshot()
        ? cache_.snapshot()
        : VdrSnapshot{};

    snapshot.channels = channels;
    cache_.update(snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateEvents(const std::vector<VdrEvent>& events)
{
    VdrSnapshot snapshot = cache_.hasSnapshot()
        ? cache_.snapshot()
        : VdrSnapshot{};

    snapshot.events = events;
    cache_.update(snapshot);
    searchTimerPreviewEpgCache_.updateReady(
        snapshot.backendId,
        events);
    incrementGeneration();
}

void SnapshotCacheService::mergeEvents(const std::vector<VdrEvent>& events)
{
    VdrSnapshot snapshot = cache_.hasSnapshot()
        ? cache_.snapshot()
        : VdrSnapshot{};

    for (const VdrEvent& event : events)
    {
        const auto existing = std::find_if(
            snapshot.events.begin(),
            snapshot.events.end(),
            [&event](const VdrEvent& cachedEvent) {
                return cachedEvent.id == event.id;
            });

        if (existing == snapshot.events.end())
        {
            snapshot.events.push_back(event);
        }
        else
        {
            *existing = event;
        }
    }

    cache_.update(snapshot);
    searchTimerPreviewEpgCache_.updateReady(
        snapshot.backendId,
        snapshot.events);
    incrementGeneration();
}

void SnapshotCacheService::updateSnapshotForBackend(
    const std::string& backendId,
    const VdrSnapshot& snapshot)
{
    cache_.updateForBackend(backendId, snapshot);
    updatePreviewCacheFromSnapshotIfEventsPresent(
        backendId,
        snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateStatusForBackend(
    const std::string& backendId,
    const VdrStatus& status)
{
    VdrSnapshot snapshot = snapshotForBackendOrEmpty(backendId);
    snapshot.status = status;
    cache_.updateForBackend(backendId, snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateRecordingsForBackend(
    const std::string& backendId,
    const std::vector<VdrRecording>& recordings)
{
    VdrSnapshot snapshot = snapshotForBackendOrEmpty(backendId);
    snapshot.recordings = recordings;
    cache_.updateForBackend(backendId, snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateTimersForBackend(
    const std::string& backendId,
    const std::vector<VdrTimer>& timers)
{
    VdrSnapshot snapshot = snapshotForBackendOrEmpty(backendId);
    snapshot.timers = timers;
    cache_.updateForBackend(backendId, snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateChannelsForBackend(
    const std::string& backendId,
    const std::vector<VdrChannel>& channels)
{
    VdrSnapshot snapshot = snapshotForBackendOrEmpty(backendId);
    snapshot.channels = channels;
    cache_.updateForBackend(backendId, snapshot);
    incrementGeneration();
}

void SnapshotCacheService::updateEventsForBackend(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    VdrSnapshot snapshot = snapshotForBackendOrEmpty(backendId);
    snapshot.events = events;
    cache_.updateForBackend(backendId, snapshot);
    searchTimerPreviewEpgCache_.updateReady(
        backendId,
        events);
    incrementGeneration();
}

void SnapshotCacheService::mergeEventsForBackend(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    VdrSnapshot snapshot = snapshotForBackendOrEmpty(backendId);

    for (const VdrEvent& event : events)
    {
        const auto existing = std::find_if(
            snapshot.events.begin(),
            snapshot.events.end(),
            [&event](const VdrEvent& cachedEvent) {
                return cachedEvent.id == event.id;
            });

        if (existing == snapshot.events.end())
        {
            snapshot.events.push_back(event);
        }
        else
        {
            *existing = event;
        }
    }

    cache_.updateForBackend(backendId, snapshot);
    searchTimerPreviewEpgCache_.updateReady(
        backendId,
        snapshot.events);
    incrementGeneration();
}

void SnapshotCacheService::clear()
{
    cache_.clear();
    searchTimerPreviewEpgCache_.clear();
    generation_ = 0;
}
