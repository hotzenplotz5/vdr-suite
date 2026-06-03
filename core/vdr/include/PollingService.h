#ifndef POLLING_SERVICE_H
#define POLLING_SERVICE_H

#include "VdrSnapshot.h"

class VdrSnapshotBuilder;

class PollingService {
public:
    explicit PollingService(VdrSnapshotBuilder& snapshotBuilder);

    void poll();

    const VdrSnapshot& snapshot() const;

private:
    VdrSnapshotBuilder& snapshotBuilder_;
    VdrSnapshot snapshot_;
};

#endif
