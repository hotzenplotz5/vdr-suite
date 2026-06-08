#ifndef SNAPSHOT_CACHE_SERVICE_H
#define SNAPSHOT_CACHE_SERVICE_H

#include "SnapshotCache.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrSnapshot.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <vector>

class SnapshotCacheService {
public:
    explicit SnapshotCacheService(SnapshotCache& cache);

    SnapshotCache& cache();
    const SnapshotCache& cache() const;

    void updateSnapshot(const VdrSnapshot& snapshot);
    void updateStatus(const VdrStatus& status);
    void updateRecordings(const std::vector<VdrRecording>& recordings);
    void updateTimers(const std::vector<VdrTimer>& timers);
    void updateChannels(const std::vector<VdrChannel>& channels);
    void updateEvents(const std::vector<VdrEvent>& events);
    void clear();

private:
    SnapshotCache& cache_;
};

#endif
