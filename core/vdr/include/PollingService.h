#ifndef POLLING_SERVICE_H
#define POLLING_SERVICE_H

#include "ChangeDetectionService.h"
#include "IRuntimeLogger.h"
#include "IRuntimeMeasurementSink.h"
#include "RuntimeLogEntry.h"
#include "RuntimeLogLevel.h"
#include "RuntimeMeasurement.h"
#include "SnapshotCacheService.h"
#include "SnapshotRefreshPlanner.h"
#include "SnapshotUpdatePlan.h"
#include "VdrChangeEvent.h"
#include "VdrChangeState.h"
#include "VdrSnapshot.h"

#include <string>
#include <vector>

class VdrService;
class VdrSnapshotBuilder;
class PollingService {

public:
    PollingService(
        VdrSnapshotBuilder& snapshotBuilder,
        VdrService& vdrService,
        SnapshotCacheService& snapshotCacheService,
        IRuntimeLogger* logger = nullptr,
        IRuntimeMeasurementSink* measurementSink = nullptr);

    void poll();

    const VdrSnapshot& snapshot() const;
    const std::vector<VdrChangeEvent>& changeEvents() const;
    const SnapshotUpdatePlan& lastUpdatePlan() const;

private:
    VdrSnapshotBuilder& snapshotBuilder_;
    VdrService& vdrService_;
    SnapshotCacheService& snapshotCacheService_;
    IRuntimeLogger* logger_;
    IRuntimeMeasurementSink* measurementSink_;

    void log(RuntimeLogLevel level, const std::string& message) const;
    void recordMeasurement(const RuntimeMeasurement& measurement) const;
    ChangeDetectionService changeDetectionService_;
    SnapshotRefreshPlanner snapshotRefreshPlanner_;
    VdrChangeState lastChangeState_;
    bool hasChangeState_;
    std::vector<VdrChangeEvent> changeEvents_;
    SnapshotUpdatePlan lastUpdatePlan_;
};

#endif
