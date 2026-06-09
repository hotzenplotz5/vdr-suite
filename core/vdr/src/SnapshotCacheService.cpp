#include "SnapshotCacheService.h"

SnapshotCacheService::SnapshotCacheService(SnapshotCache& cache)
    : cache_(cache),
      generation_(0)
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

void SnapshotCacheService::updateSnapshot(const VdrSnapshot& snapshot)
{
    cache_.update(snapshot);
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
    incrementGeneration();
}

void SnapshotCacheService::clear()
{
    cache_.clear();
    generation_ = 0;
}
