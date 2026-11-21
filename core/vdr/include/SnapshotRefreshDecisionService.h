#ifndef SNAPSHOT_REFRESH_DECISION_SERVICE_H
#define SNAPSHOT_REFRESH_DECISION_SERVICE_H

#include "SnapshotRefreshDecision.h"
#include "VdrChangeEvent.h"

#include <vector>

class SnapshotRefreshDecisionService {
public:
    SnapshotRefreshDecision decide(const std::vector<VdrChangeEvent>& changeEvents) const;
};

#endif
