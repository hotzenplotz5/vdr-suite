#ifndef SNAPSHOT_CACHE_H
#define SNAPSHOT_CACHE_H

#include "VdrSnapshot.h"

class SnapshotCache {
public:
    SnapshotCache();

    bool hasSnapshot() const;
    const VdrSnapshot& snapshot() const;
    void update(const VdrSnapshot& snapshot);
    void clear();

private:
    bool hasSnapshot_;
    VdrSnapshot snapshot_;
};

#endif
