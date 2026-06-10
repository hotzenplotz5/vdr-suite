#ifndef SNAPSHOT_ACCESS_SERVICE_H
#define SNAPSHOT_ACCESS_SERVICE_H

#include "ISnapshotAccessService.h"
#include "SnapshotCacheService.h"

#include <string>

class SnapshotAccessService : public ISnapshotAccessService {
public:
    explicit SnapshotAccessService(SnapshotCacheService& cacheService);

    bool hasSnapshot() const override;
    const VdrSnapshot* snapshot() const override;

    bool hasSnapshotForBackend(const std::string& backendId) const override;
    const VdrSnapshot* snapshotForBackend(const std::string& backendId) const override;

private:
    SnapshotCacheService& cacheService_;
};

#endif
