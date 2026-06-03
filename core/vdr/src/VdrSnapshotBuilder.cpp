#include "VdrSnapshotBuilder.h"

#include "VdrService.h"

VdrSnapshotBuilder::VdrSnapshotBuilder(VdrService& vdrService)
    : vdrService_(vdrService)
{
}

VdrSnapshot VdrSnapshotBuilder::buildSnapshot() const
{
    VdrSnapshot snapshot;

    snapshot.status = vdrService_.getStatus();
    snapshot.recordings = vdrService_.getRecordings();
    snapshot.timers = vdrService_.getTimers();
    snapshot.channels = vdrService_.getChannels();
    snapshot.events = vdrService_.getEvents();

    return snapshot;
}
