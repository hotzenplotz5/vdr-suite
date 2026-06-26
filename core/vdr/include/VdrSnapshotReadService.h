#pragma once

#include "ISnapshotAccessService.h"
#include "SearchTimer.h"
#include "SearchTimerPreviewEpgCache.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <string>
#include <vector>

class VdrSnapshotReadService
{
public:
    explicit VdrSnapshotReadService(
        ISnapshotAccessService& snapshotAccessService);

    bool hasSnapshot() const;
    bool hasSnapshotForBackend(const std::string& backendId) const;
    std::vector<VdrSnapshot> getSnapshots() const;

    VdrStatus getStatus() const;
    VdrStatus getStatusForBackend(const std::string& backendId) const;

    std::vector<VdrRecording> getRecordings() const;
    std::vector<VdrRecording> getRecordingsForBackend(const std::string& backendId) const;

    std::vector<VdrTimer> getTimers() const;
    std::vector<VdrTimer> getTimersForBackend(const std::string& backendId) const;

    std::vector<SearchTimer> getSearchTimers() const;
    std::vector<SearchTimer> getSearchTimersForBackend(const std::string& backendId) const;

    std::vector<VdrChannel> getChannels() const;
    std::vector<VdrChannel> getChannelsForBackend(const std::string& backendId) const;

    std::vector<VdrEvent> getEvents() const;
    std::vector<VdrEvent> getEventsForBackend(const std::string& backendId) const;

    SearchTimerPreviewEpgCache& searchTimerPreviewEpgCache();
    const SearchTimerPreviewEpgCache& searchTimerPreviewEpgCache() const;

private:
    ISnapshotAccessService& snapshotAccessService_;
    SearchTimerPreviewEpgCache searchTimerPreviewEpgCache_;
};
