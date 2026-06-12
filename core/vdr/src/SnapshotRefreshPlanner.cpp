#include "SnapshotRefreshPlanner.h"

SnapshotRefreshPlanner::SnapshotRefreshPlanner()
    : refreshPolicy_()
{
}

SnapshotRefreshPlanner::SnapshotRefreshPlanner(DomainRefreshPolicy refreshPolicy)
    : refreshPolicy_(refreshPolicy)
{
}

SnapshotUpdatePlan SnapshotRefreshPlanner::createPlan(
    const std::vector<VdrChangeEvent>& changeEvents) const
{
    SnapshotUpdatePlan plan;

    for (const auto& event : changeEvents) {
        switch (event.type()) {
        case VdrChangeType::StatusChanged:
            if (refreshPolicy_.allowsAutomaticFullRefresh(RefreshDomain::Status)) {
                plan.markStatusRefresh();
            }
            break;
        case VdrChangeType::ChannelsChanged:
            if (refreshPolicy_.allowsAutomaticFullRefresh(RefreshDomain::Channels)) {
                plan.markChannelsRefresh();
            }
            break;
        case VdrChangeType::RecordingsChanged:
            if (refreshPolicy_.allowsAutomaticFullRefresh(RefreshDomain::Recordings)) {
                plan.markRecordingsRefresh();
            }
            break;
        case VdrChangeType::TimersChanged:
            if (refreshPolicy_.allowsAutomaticFullRefresh(RefreshDomain::Timers)) {
                plan.markTimersRefresh();
            }
            break;
        case VdrChangeType::EventsChanged:
            if (refreshPolicy_.allowsAutomaticFullRefresh(RefreshDomain::Events)) {
                plan.markEventsRefresh();
            } else if (refreshPolicy_.requiresSelectiveRefresh(RefreshDomain::Events)) {
                VdrEventQuery query;
                query.channelEventLimit = 2;
                plan.markSelectiveEventRefresh(query);
            }
            break;
        }
    }

    return plan;
}
