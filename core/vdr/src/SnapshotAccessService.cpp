#include "SnapshotAccessService.h"

SnapshotAccessService::SnapshotAccessService(
    SnapshotCacheService& cacheService)
    : cacheService_(cacheService)
{
}

bool SnapshotAccessService::hasSnapshot() const
{
    return cacheService_.cache().hasSnapshot();
}

const VdrSnapshot* SnapshotAccessService::snapshot() const
{
    if (!cacheService_.cache().hasSnapshot()) {
        return nullptr;
    }

    return &cacheService_.cache().snapshot();
}
