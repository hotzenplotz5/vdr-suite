#include "PollingService.h"

#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

PollingService::PollingService(
    VdrSnapshotBuilder& snapshotBuilder,
    VdrService& vdrService,
    SnapshotCacheService& snapshotCacheService)
    : snapshotBuilder_(snapshotBuilder),
      vdrService_(vdrService),
      snapshotCacheService_(snapshotCacheService),
      hasChangeState_(false)
{
}

void PollingService::poll()
{
    const VdrChangeState currentChangeState = vdrService_.getChangeState();

    changeEvents_.clear();
    lastUpdatePlan_ = SnapshotUpdatePlan();

    if (!hasChangeState_) {
        snapshotCacheService_.updateSnapshot(snapshotBuilder_.buildSnapshot());
        lastChangeState_ = currentChangeState;
        hasChangeState_ = true;
        return;
    }

    changeEvents_ = changeDetectionService_.detectChanges(lastChangeState_, currentChangeState);
    lastUpdatePlan_ = snapshotRefreshPlanner_.createPlan(changeEvents_);

    if (lastUpdatePlan_.hasRefreshWork()) {
        snapshotCacheService_.updateSnapshot(snapshotBuilder_.buildSnapshot());
        lastChangeState_ = currentChangeState;
    }
}

const VdrSnapshot& PollingService::snapshot() const
{
    return snapshotCacheService_.cache().snapshot();
}

const std::vector<VdrChangeEvent>& PollingService::changeEvents() const
{
    return changeEvents_;
}

const SnapshotUpdatePlan& PollingService::lastUpdatePlan() const
{
    return lastUpdatePlan_;
}
