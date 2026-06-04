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

    if (!hasChangeState_) {
        snapshotCacheService_.cache().update(snapshotBuilder_.buildSnapshot());
        lastChangeState_ = currentChangeState;
        hasChangeState_ = true;
        return;
    }

    changeEvents_ = changeDetectionService_.detectChanges(lastChangeState_, currentChangeState);

    const auto refreshDecision = snapshotRefreshDecisionService_.decide(changeEvents_);

    if (refreshDecision.shouldRefreshSnapshot()) {
        snapshotCacheService_.cache().update(snapshotBuilder_.buildSnapshot());
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
