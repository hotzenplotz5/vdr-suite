#include "SnapshotRefreshPlanner.h"
#include "VdrChangeEvent.h"

#include <cassert>
#include <iostream>
#include <vector>

static void test_empty_events_create_empty_plan()
{
    SnapshotRefreshPlanner planner;

    const auto plan = planner.createPlan({});

    assert(plan.hasRefreshWork() == false);
    assert(plan.requiresFullSnapshot() == false);
}

static void test_single_domain_event_creates_matching_domain_refresh()
{
    SnapshotRefreshPlanner planner;

    const auto plan = planner.createPlan({
        VdrChangeEvent(VdrChangeType::ChannelsChanged)
    });

    assert(plan.hasRefreshWork() == true);
    assert(plan.requiresFullSnapshot() == false);
    assert(plan.shouldRefreshStatus() == false);
    assert(plan.shouldRefreshChannels() == true);
    assert(plan.shouldRefreshRecordings() == false);
    assert(plan.shouldRefreshTimers() == false);
    assert(plan.shouldRefreshEvents() == false);
    assert(plan.hasSelectiveEventRefresh() == false);
}

static void test_multiple_lightweight_domain_events_are_merged_into_one_plan()
{
    SnapshotRefreshPlanner planner;

    const auto plan = planner.createPlan({
        VdrChangeEvent(VdrChangeType::RecordingsChanged),
        VdrChangeEvent(VdrChangeType::TimersChanged)
    });

    assert(plan.hasRefreshWork() == true);
    assert(plan.requiresFullSnapshot() == false);
    assert(plan.shouldRefreshStatus() == false);
    assert(plan.shouldRefreshChannels() == false);
    assert(plan.shouldRefreshRecordings() == true);
    assert(plan.shouldRefreshTimers() == true);
    assert(plan.shouldRefreshEvents() == false);
}

static void test_events_changed_creates_selective_event_refresh_by_default()
{
    SnapshotRefreshPlanner planner;

    const auto plan = planner.createPlan({
        VdrChangeEvent(VdrChangeType::EventsChanged)
    });

    const auto query = plan.selectiveEventQuery();

    assert(plan.hasRefreshWork() == true);
    assert(plan.requiresFullSnapshot() == false);
    assert(plan.shouldRefreshEvents() == false);
    assert(plan.hasSelectiveEventRefresh() == true);
    assert(query.channelEventLimit == 2);
}

static void test_mixed_events_change_keeps_lightweight_refreshes_only()
{
    SnapshotRefreshPlanner planner;

    const auto plan = planner.createPlan({
        VdrChangeEvent(VdrChangeType::RecordingsChanged),
        VdrChangeEvent(VdrChangeType::EventsChanged),
        VdrChangeEvent(VdrChangeType::TimersChanged)
    });

    assert(plan.hasRefreshWork() == true);
    assert(plan.requiresFullSnapshot() == false);
    assert(plan.shouldRefreshRecordings() == true);
    assert(plan.shouldRefreshTimers() == true);
    assert(plan.shouldRefreshEvents() == false);
}

static void test_status_event_creates_status_refresh()
{
    SnapshotRefreshPlanner planner;

    const auto plan = planner.createPlan({
        VdrChangeEvent(VdrChangeType::StatusChanged)
    });

    assert(plan.hasRefreshWork() == true);
    assert(plan.shouldRefreshStatus() == true);
    assert(plan.shouldRefreshChannels() == false);
    assert(plan.shouldRefreshRecordings() == false);
    assert(plan.shouldRefreshTimers() == false);
    assert(plan.shouldRefreshEvents() == false);
}

int main()
{
    test_empty_events_create_empty_plan();
    test_single_domain_event_creates_matching_domain_refresh();
    test_multiple_lightweight_domain_events_are_merged_into_one_plan();
    test_events_changed_creates_selective_event_refresh_by_default();
    test_mixed_events_change_keeps_lightweight_refreshes_only();
    test_status_event_creates_status_refresh();

    std::cout
        << "test_snapshot_refresh_planner passed"
        << std::endl;

    return 0;
}
