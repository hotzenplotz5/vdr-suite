#include "VdrSnapshotBuilder.h"

#include "VdrService.h"

VdrSnapshotBuilder::VdrSnapshotBuilder(VdrService& vdrService)
    : vdrService_(vdrService)
{
}

VdrStatus VdrSnapshotBuilder::buildStatus() const
{
    return vdrService_.getStatus();
}

std::vector<VdrRecording> VdrSnapshotBuilder::buildRecordings() const
{
    return vdrService_.getRecordings();
}

std::vector<VdrTimer> VdrSnapshotBuilder::buildTimers() const
{
    return vdrService_.getTimers();
}

std::vector<VdrChannel> VdrSnapshotBuilder::buildChannels() const
{
    return vdrService_.getChannels();
}

std::vector<VdrEvent> VdrSnapshotBuilder::buildEvents() const
{
    return vdrService_.getEvents();
}

VdrSnapshot VdrSnapshotBuilder::buildSnapshot() const
{
    VdrSnapshot snapshot;

    snapshot.status = buildStatus();
    snapshot.recordings = buildRecordings();
    snapshot.timers = buildTimers();
    snapshot.channels = buildChannels();
    snapshot.events = buildEvents();

    return snapshot;
}
