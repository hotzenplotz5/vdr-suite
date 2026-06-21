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

std::vector<VdrSnapshot> VdrSnapshotReadService::getSnapshots() const
{
    return snapshotAccessService_.snapshots();
}

bool VdrSnapshotReadService::hasSnapshotForBackend(
    const std::string& backendId) const
{
    return snapshotAccessService_.hasSnapshotForBackend(backendId);
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

VdrStatus VdrSnapshotReadService::getStatusForBackend(
    const std::string& backendId) const
{
    const auto snapshot =
        snapshotAccessService_.snapshotForBackend(backendId);

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

std::vector<VdrRecording> VdrSnapshotReadService::getRecordingsForBackend(
    const std::string& backendId) const
{
    const auto snapshot =
        snapshotAccessService_.snapshotForBackend(backendId);

    return snapshot ? snapshot->recordings : std::vector<VdrRecording>{};
}

std::vector<VdrTimer> VdrSnapshotReadService::getTimers() const
{
    const auto snapshot = snapshotAccessService_.snapshot();
    return snapshot ? snapshot->timers : std::vector<VdrTimer>{};
}

std::vector<VdrTimer> VdrSnapshotReadService::getTimersForBackend(
    const std::string& backendId) const
{
    const auto snapshot =
        snapshotAccessService_.snapshotForBackend(backendId);

    return snapshot ? snapshot->timers : std::vector<VdrTimer>{};
}

std::vector<SearchTimer> VdrSnapshotReadService::getSearchTimers() const
{
    const auto snapshot = snapshotAccessService_.snapshot();
    return snapshot ? snapshot->searchTimers : std::vector<SearchTimer>{};
}

std::vector<SearchTimer> VdrSnapshotReadService::getSearchTimersForBackend(
    const std::string& backendId) const
{
    const auto snapshot =
        snapshotAccessService_.snapshotForBackend(backendId);

    return snapshot ? snapshot->searchTimers : std::vector<SearchTimer>{};
}

std::vector<VdrChannel> VdrSnapshotReadService::getChannels() const
{
    const auto snapshot = snapshotAccessService_.snapshot();
    return snapshot ? snapshot->channels : std::vector<VdrChannel>{};
}

std::vector<VdrChannel> VdrSnapshotReadService::getChannelsForBackend(
    const std::string& backendId) const
{
    const auto snapshot =
        snapshotAccessService_.snapshotForBackend(backendId);

    return snapshot ? snapshot->channels : std::vector<VdrChannel>{};
}

std::vector<VdrEvent> VdrSnapshotReadService::getEvents() const
{
    const auto snapshot = snapshotAccessService_.snapshot();
    return snapshot ? snapshot->events : std::vector<VdrEvent>{};
}

std::vector<VdrEvent> VdrSnapshotReadService::getEventsForBackend(
    const std::string& backendId) const
{
    const auto snapshot =
        snapshotAccessService_.snapshotForBackend(backendId);

    return snapshot ? snapshot->events : std::vector<VdrEvent>{};
}
