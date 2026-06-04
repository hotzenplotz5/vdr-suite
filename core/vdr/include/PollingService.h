#ifndef POLLING_SERVICE_H
#define POLLING_SERVICE_H

#include "ChangeDetectionService.h"
#include "SnapshotCacheService.h"
#include "IRuntimeLogger.h"
#include "RuntimeLogEntry.h"
#include "RuntimeLogLevel.h"
#include "SnapshotRefreshPlanner.h"
#include "SnapshotUpdatePlan.h"
#include "VdrChangeEvent.h"
#include "VdrChangeState.h"
#include "VdrSnapshot.h"

#include <vector>

class VdrService;
class VdrSnapshotBuilder;
class PollingService {

public:
    PollingService(
        VdrSnapshotBuilder& snapshotBuilder,
        VdrService& vdrService,
        SnapshotCacheService& snapshotCacheService);

    void poll();

    const VdrSnapshot& snapshot() const;
    const std::vector<VdrChangeEvent>& changeEvents() const;
    const SnapshotUpdatePlan& lastUpdatePlan() const;

private:
    VdrSnapshotBuilder& snapshotBuilder_;
    VdrService& vdrService_;
    SnapshotCacheService& snapshotCacheService_;
    IRuntimeLogger* logger_;

    void log(RuntimeLogLevel level, const std::string& message) const;
    ChangeDetectionService changeDetectionService_;
    SnapshotRefreshPlanner snapshotRefreshPlanner_;
    VdrChangeState lastChangeState_;
    bool hasChangeState_;
    std::vector<VdrChangeEvent> changeEvents_;
    SnapshotUpdatePlan lastUpdatePlan_;
};

#endif
