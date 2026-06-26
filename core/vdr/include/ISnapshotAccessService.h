#ifndef I_SNAPSHOT_ACCESS_SERVICE_H
#define I_SNAPSHOT_ACCESS_SERVICE_H

#include "SearchTimerPreviewEpgCache.h"
#include "VdrSnapshot.h"

#include <string>
#include <vector>

class ISnapshotAccessService {
public:
    virtual ~ISnapshotAccessService() = default;

    virtual bool hasSnapshot() const = 0;
    virtual const VdrSnapshot* snapshot() const = 0;

    virtual bool hasSnapshotForBackend(const std::string& backendId) const = 0;
    virtual const VdrSnapshot* snapshotForBackend(const std::string& backendId) const = 0;
    virtual std::vector<VdrSnapshot> snapshots() const = 0;

    virtual SearchTimerPreviewEpgCache& searchTimerPreviewEpgCache() = 0;
    virtual const SearchTimerPreviewEpgCache& searchTimerPreviewEpgCache() const = 0;
};

#endif
