#ifndef SNAPSHOT_REFRESH_PLANNER_H
#define SNAPSHOT_REFRESH_PLANNER_H

#include "SnapshotUpdatePlan.h"
#include "VdrChangeEvent.h"

#include <vector>

class SnapshotRefreshPlanner {
public:
    SnapshotUpdatePlan createPlan(
        const std::vector<VdrChangeEvent>& changeEvents) const;
};

#endif
