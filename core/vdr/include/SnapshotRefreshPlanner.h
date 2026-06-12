#ifndef SNAPSHOT_REFRESH_PLANNER_H
#define SNAPSHOT_REFRESH_PLANNER_H

#include "DomainRefreshPolicy.h"
#include "SnapshotUpdatePlan.h"
#include "VdrChangeEvent.h"

#include <vector>

class SnapshotRefreshPlanner {
public:
    SnapshotRefreshPlanner();

    explicit SnapshotRefreshPlanner(DomainRefreshPolicy refreshPolicy);

    SnapshotUpdatePlan createPlan(
        const std::vector<VdrChangeEvent>& changeEvents) const;

private:
    DomainRefreshPolicy refreshPolicy_;
};

#endif
