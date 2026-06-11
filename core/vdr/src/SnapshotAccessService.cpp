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

bool SnapshotAccessService::hasSnapshotForBackend(
    const std::string& backendId) const
{
    return snapshotForBackend(backendId) != nullptr;
}

const VdrSnapshot* SnapshotAccessService::snapshotForBackend(
    const std::string& backendId) const
{
    return cacheService_.cache().snapshotForBackend(backendId);
}

std::vector<VdrSnapshot> SnapshotAccessService::snapshots() const
{
    std::vector<VdrSnapshot> result;

    for (const std::string& backendId : cacheService_.cache().backendIds())
    {
        const VdrSnapshot* backendSnapshot =
            cacheService_.cache().snapshotForBackend(backendId);

        if (backendSnapshot != nullptr)
        {
            result.push_back(*backendSnapshot);
        }
    }

    return result;
}
