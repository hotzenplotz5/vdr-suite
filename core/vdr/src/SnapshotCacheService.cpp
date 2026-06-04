#include "SnapshotCacheService.h"

SnapshotCacheService::SnapshotCacheService(SnapshotCache& cache)
    : cache_(cache)
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
