#include "SnapshotRefreshPlanner.h"

SnapshotUpdatePlan SnapshotRefreshPlanner::createPlan(
    const std::vector<VdrChangeEvent>& changeEvents) const
{
    SnapshotUpdatePlan plan;

    for (const auto& event : changeEvents) {
        switch (event.type()) {
        case VdrChangeType::StatusChanged:
            plan.markStatusRefresh();
            break;
        case VdrChangeType::ChannelsChanged:
            plan.markChannelsRefresh();
            break;
        case VdrChangeType::RecordingsChanged:
            plan.markRecordingsRefresh();
            break;
        case VdrChangeType::TimersChanged:
            plan.markTimersRefresh();
            break;
        case VdrChangeType::EventsChanged:
            plan.markEventsRefresh();
            break;
        }
    }

    return plan;
}
