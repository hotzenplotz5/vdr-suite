#ifndef SNAPSHOT_CACHE_SERVICE_H
#define SNAPSHOT_CACHE_SERVICE_H

#include "SnapshotCache.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrSnapshot.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <string>
#include <vector>

class SnapshotCacheService {
public:
    explicit SnapshotCacheService(SnapshotCache& cache);

    SnapshotCache& cache();
    const SnapshotCache& cache() const;

    int generation() const;
    std::string backendId() const;

    void updateSnapshot(const VdrSnapshot& snapshot);
    void updateStatus(const VdrStatus& status);
    void updateRecordings(const std::vector<VdrRecording>& recordings);
    void updateTimers(const std::vector<VdrTimer>& timers);
    void updateChannels(const std::vector<VdrChannel>& channels);
    void updateEvents(const std::vector<VdrEvent>& events);
    void mergeEvents(const std::vector<VdrEvent>& events);

    void updateSnapshotForBackend(const std::string& backendId, const VdrSnapshot& snapshot);
    void updateStatusForBackend(const std::string& backendId, const VdrStatus& status);
    void updateRecordingsForBackend(const std::string& backendId, const std::vector<VdrRecording>& recordings);
    void updateTimersForBackend(const std::string& backendId, const std::vector<VdrTimer>& timers);
    void updateChannelsForBackend(const std::string& backendId, const std::vector<VdrChannel>& channels);
    void updateEventsForBackend(const std::string& backendId, const std::vector<VdrEvent>& events);
    void mergeEventsForBackend(const std::string& backendId, const std::vector<VdrEvent>& events);

    void clear();

private:
    void incrementGeneration();
    VdrSnapshot snapshotForBackendOrEmpty(const std::string& backendId) const;

    SnapshotCache& cache_;
    int generation_;
};

#endif
