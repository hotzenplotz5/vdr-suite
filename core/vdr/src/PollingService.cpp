#include "PollingService.h"

#include "VdrSnapshotBuilder.h"

PollingService::PollingService(VdrSnapshotBuilder& snapshotBuilder)
    : snapshotBuilder_(snapshotBuilder)
{
}

void PollingService::poll()
{
    snapshot_ = snapshotBuilder_.buildSnapshot();
}

const VdrSnapshot& PollingService::snapshot() const
{
    return snapshot_;
}
