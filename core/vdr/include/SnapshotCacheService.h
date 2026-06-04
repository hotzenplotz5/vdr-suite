#ifndef SNAPSHOT_CACHE_SERVICE_H
#define SNAPSHOT_CACHE_SERVICE_H

#include "SnapshotCache.h"

class SnapshotCacheService {
public:
    explicit SnapshotCacheService(SnapshotCache& cache);

    SnapshotCache& cache();
    const SnapshotCache& cache() const;

private:
    SnapshotCache& cache_;
};

#endif
