#include "SnapshotCache.h"

SnapshotCache::SnapshotCache()
    : hasSnapshot_(false)
{
}

bool SnapshotCache::hasSnapshot() const
{
    return hasSnapshot_;
}

const VdrSnapshot& SnapshotCache::snapshot() const
{
    return snapshot_;
}

void SnapshotCache::update(const VdrSnapshot& snapshot)
{
    snapshot_ = snapshot;
    hasSnapshot_ = true;
}

void SnapshotCache::clear()
{
    snapshot_ = VdrSnapshot{};
    hasSnapshot_ = false;
}
