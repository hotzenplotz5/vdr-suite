#include "PollingService.h"

#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

PollingService::PollingService(
    VdrSnapshotBuilder& snapshotBuilder,
    VdrService& vdrService,
    SnapshotCacheService& snapshotCacheService,
    IRuntimeLogger* logger)
    : snapshotBuilder_(snapshotBuilder),
      vdrService_(vdrService),
      snapshotCacheService_(snapshotCacheService),
      logger_(logger),
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

    if (lastUpdatePlan_.requiresFullSnapshot()) {
        snapshotCacheService_.updateSnapshot(snapshotBuilder_.buildSnapshot());
        lastChangeState_ = currentChangeState;
        return;
    }

    if (lastUpdatePlan_.hasRefreshWork()) {
        if (lastUpdatePlan_.shouldRefreshStatus()) {
            snapshotCacheService_.updateStatus(snapshotBuilder_.buildStatus());
        }

        if (lastUpdatePlan_.shouldRefreshRecordings()) {
            snapshotCacheService_.updateRecordings(snapshotBuilder_.buildRecordings());
        }

        if (lastUpdatePlan_.shouldRefreshTimers()) {
            snapshotCacheService_.updateTimers(snapshotBuilder_.buildTimers());
        }

        if (lastUpdatePlan_.shouldRefreshChannels()) {
            snapshotCacheService_.updateChannels(snapshotBuilder_.buildChannels());
        }

        if (lastUpdatePlan_.shouldRefreshEvents()) {
            snapshotCacheService_.updateEvents(snapshotBuilder_.buildEvents());
        }

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
