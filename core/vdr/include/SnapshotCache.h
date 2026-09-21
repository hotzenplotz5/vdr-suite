#ifndef SNAPSHOT_CACHE_H
#define SNAPSHOT_CACHE_H

#include "VdrSnapshot.h"

#include <map>
#include <string>
#include <vector>

class SnapshotCache {
public:
    SnapshotCache();

    bool hasSnapshot() const;
    const VdrSnapshot& snapshot() const;
    void update(const VdrSnapshot& snapshot);
    void clear();

    bool hasSnapshotForBackend(const std::string& backendId) const;
    const VdrSnapshot* snapshotForBackend(const std::string& backendId) const;
    void updateForBackend(const std::string& backendId, const VdrSnapshot& snapshot);
    void clearBackend(const std::string& backendId);
    std::vector<std::string> backendIds() const;

private:
    bool hasSnapshot_;
    VdrSnapshot snapshot_;
    std::map<std::string, VdrSnapshot> snapshotsByBackend_;
};

#endif
