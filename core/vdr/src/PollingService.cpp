#include "PollingService.h"

#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

PollingService::PollingService(VdrSnapshotBuilder& snapshotBuilder, VdrService& vdrService)
    : snapshotBuilder_(snapshotBuilder),
      vdrService_(vdrService),
      hasChangeState_(false)
{
}

void PollingService::poll()
{
    const VdrChangeState currentChangeState = vdrService_.getChangeState();

    if (!hasChangeState_) {
        snapshot_ = snapshotBuilder_.buildSnapshot();
        lastChangeState_ = currentChangeState;
        hasChangeState_ = true;
        return;
    }

    const auto changes = changeDetectionService_.detectChanges(lastChangeState_, currentChangeState);

    if (!changes.empty()) {
        snapshot_ = snapshotBuilder_.buildSnapshot();
        lastChangeState_ = currentChangeState;
    }
}

const VdrSnapshot& PollingService::snapshot() const
{
    return snapshot_;
}
