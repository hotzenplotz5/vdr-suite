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
}

static void test_multiple_domain_events_are_merged_into_one_plan()
{
    SnapshotRefreshPlanner planner;

    const auto plan = planner.createPlan({
        VdrChangeEvent(VdrChangeType::RecordingsChanged),
        VdrChangeEvent(VdrChangeType::TimersChanged),
        VdrChangeEvent(VdrChangeType::EventsChanged)
    });

    assert(plan.hasRefreshWork() == true);
    assert(plan.requiresFullSnapshot() == false);
    assert(plan.shouldRefreshStatus() == false);
    assert(plan.shouldRefreshChannels() == false);
    assert(plan.shouldRefreshRecordings() == true);
    assert(plan.shouldRefreshTimers() == true);
    assert(plan.shouldRefreshEvents() == true);
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
    test_multiple_domain_events_are_merged_into_one_plan();
    test_status_event_creates_status_refresh();

    std::cout
        << "test_snapshot_refresh_planner passed"
        << std::endl;

    return 0;
}
