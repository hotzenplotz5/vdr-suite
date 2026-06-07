#pragma once

#include "ISnapshotAccessService.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <vector>

class VdrSnapshotReadService
{
public:
    explicit VdrSnapshotReadService(
        ISnapshotAccessService& snapshotAccessService);

    bool hasSnapshot() const;
    VdrStatus getStatus() const;
    std::vector<VdrRecording> getRecordings() const;
    std::vector<VdrTimer> getTimers() const;
    std::vector<VdrChannel> getChannels() const;
    std::vector<VdrEvent> getEvents() const;

private:
    ISnapshotAccessService& snapshotAccessService_;
};
