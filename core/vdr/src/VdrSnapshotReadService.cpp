#include "VdrSnapshotReadService.h"

VdrSnapshotReadService::VdrSnapshotReadService(
    ISnapshotAccessService& snapshotAccessService)
    : snapshotAccessService_(snapshotAccessService)
{
}

bool VdrSnapshotReadService::hasSnapshot() const
{
    return snapshotAccessService_.hasSnapshot();
}

VdrStatus VdrSnapshotReadService::getStatus() const
{
    const auto snapshot = snapshotAccessService_.snapshot();

    if (!snapshot)
    {
        return {false, "", "", 0, ""};
    }

    return snapshot->status;
}

std::vector<VdrRecording> VdrSnapshotReadService::getRecordings() const
{
    const auto snapshot = snapshotAccessService_.snapshot();
    return snapshot ? snapshot->recordings : std::vector<VdrRecording>{};
}

std::vector<VdrTimer> VdrSnapshotReadService::getTimers() const
{
    const auto snapshot = snapshotAccessService_.snapshot();
    return snapshot ? snapshot->timers : std::vector<VdrTimer>{};
}

std::vector<VdrChannel> VdrSnapshotReadService::getChannels() const
{
    const auto snapshot = snapshotAccessService_.snapshot();
    return snapshot ? snapshot->channels : std::vector<VdrChannel>{};
}

std::vector<VdrEvent> VdrSnapshotReadService::getEvents() const
{
    const auto snapshot = snapshotAccessService_.snapshot();
    return snapshot ? snapshot->events : std::vector<VdrEvent>{};
}
