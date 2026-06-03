#ifndef VDR_SNAPSHOT_BUILDER_H
#define VDR_SNAPSHOT_BUILDER_H

#include "VdrSnapshot.h"

class VdrService;

class VdrSnapshotBuilder {
public:
    explicit VdrSnapshotBuilder(VdrService& vdrService);

    VdrSnapshot buildSnapshot() const;

private:
    VdrService& vdrService_;
};

#endif
