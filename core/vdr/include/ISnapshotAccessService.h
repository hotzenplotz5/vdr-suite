#ifndef I_SNAPSHOT_ACCESS_SERVICE_H
#define I_SNAPSHOT_ACCESS_SERVICE_H

#include "VdrSnapshot.h"

class ISnapshotAccessService {
public:
    virtual ~ISnapshotAccessService() = default;

    virtual bool hasSnapshot() const = 0;
    virtual const VdrSnapshot* snapshot() const = 0;
};

#endif
