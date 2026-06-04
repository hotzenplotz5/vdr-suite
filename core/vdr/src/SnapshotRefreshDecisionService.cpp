#include "SnapshotRefreshDecisionService.h"

SnapshotRefreshDecision SnapshotRefreshDecisionService::decide(
    const std::vector<VdrChangeEvent>& changeEvents) const
{
    if (changeEvents.empty()) {
        return SnapshotRefreshDecision::noRefresh();
    }

    return SnapshotRefreshDecision::fullRefresh();
}
