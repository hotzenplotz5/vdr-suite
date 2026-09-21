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

    const std::string backendId =
        snapshot.backendId.empty()
            ? "default"
            : snapshot.backendId;

    snapshotsByBackend_[backendId] = snapshot_;
    snapshotsByBackend_[backendId].backendId = backendId;
}

void SnapshotCache::clear()
{
    snapshot_ = VdrSnapshot{};
    hasSnapshot_ = false;
    snapshotsByBackend_.clear();
}

bool SnapshotCache::hasSnapshotForBackend(
    const std::string& backendId) const
{
    return snapshotsByBackend_.find(backendId) != snapshotsByBackend_.end();
}

const VdrSnapshot* SnapshotCache::snapshotForBackend(
    const std::string& backendId) const
{
    const auto it = snapshotsByBackend_.find(backendId);

    if (it == snapshotsByBackend_.end())
    {
        return nullptr;
    }

    return &it->second;
}

void SnapshotCache::updateForBackend(
    const std::string& backendId,
    const VdrSnapshot& snapshot)
{
    VdrSnapshot backendSnapshot = snapshot;
    backendSnapshot.backendId = backendId;

    snapshotsByBackend_[backendId] = backendSnapshot;

    snapshot_ = backendSnapshot;
    hasSnapshot_ = true;
}

void SnapshotCache::clearBackend(
    const std::string& backendId)
{
    snapshotsByBackend_.erase(backendId);

    if (snapshot_.backendId == backendId)
    {
        snapshot_ = VdrSnapshot{};
        hasSnapshot_ = false;
    }
}

std::vector<std::string> SnapshotCache::backendIds() const
{
    std::vector<std::string> ids;

    for (const auto& entry : snapshotsByBackend_)
    {
        ids.push_back(entry.first);
    }

    return ids;
}
