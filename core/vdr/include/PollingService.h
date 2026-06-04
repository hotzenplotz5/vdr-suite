#ifndef POLLING_SERVICE_H
#define POLLING_SERVICE_H

#include "ChangeDetectionService.h"
#include "VdrChangeEvent.h"
#include "VdrChangeState.h"
#include "VdrSnapshot.h"

#include <vector>

class VdrService;
class VdrSnapshotBuilder;

class PollingService {
public:
    PollingService(VdrSnapshotBuilder& snapshotBuilder, VdrService& vdrService);

    void poll();

    const VdrSnapshot& snapshot() const;
    const std::vector<VdrChangeEvent>& changeEvents() const;

private:
    VdrSnapshotBuilder& snapshotBuilder_;
    VdrService& vdrService_;
    ChangeDetectionService changeDetectionService_;
    VdrChangeState lastChangeState_;
    bool hasChangeState_;
    VdrSnapshot snapshot_;
    std::vector<VdrChangeEvent> changeEvents_;
};

#endif
