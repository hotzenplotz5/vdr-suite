#ifndef SNAPSHOT_ACCESS_SERVICE_H
#define SNAPSHOT_ACCESS_SERVICE_H

#include "ISnapshotAccessService.h"
#include "SnapshotCacheService.h"

class SnapshotAccessService : public ISnapshotAccessService {
public:
    explicit SnapshotAccessService(SnapshotCacheService& cacheService);

    bool hasSnapshot() const override;
    const VdrSnapshot* snapshot() const override;

private:
    SnapshotCacheService& cacheService_;
};

#endif
