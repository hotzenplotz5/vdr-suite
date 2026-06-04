#ifndef VDR_SNAPSHOT_BUILDER_H
#define VDR_SNAPSHOT_BUILDER_H

#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrSnapshot.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <vector>

class VdrService;

class VdrSnapshotBuilder {
public:
    explicit VdrSnapshotBuilder(VdrService& vdrService);

    VdrStatus buildStatus() const;
    std::vector<VdrRecording> buildRecordings() const;
    std::vector<VdrTimer> buildTimers() const;
    std::vector<VdrChannel> buildChannels() const;
    std::vector<VdrEvent> buildEvents() const;

    VdrSnapshot buildSnapshot() const;

private:
    VdrService& vdrService_;
};

#endif
