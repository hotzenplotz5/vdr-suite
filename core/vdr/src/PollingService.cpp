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

    changeEvents_.clear();

    if (!hasChangeState_) {
        snapshot_ = snapshotBuilder_.buildSnapshot();
        lastChangeState_ = currentChangeState;
        hasChangeState_ = true;
        return;
    }

    changeEvents_ = changeDetectionService_.detectChanges(lastChangeState_, currentChangeState);

    if (!changeEvents_.empty()) {
        snapshot_ = snapshotBuilder_.buildSnapshot();
        lastChangeState_ = currentChangeState;
    }
}

const VdrSnapshot& PollingService::snapshot() const
{
    return snapshot_;
}

const std::vector<VdrChangeEvent>& PollingService::changeEvents() const
{
    return changeEvents_;
}
