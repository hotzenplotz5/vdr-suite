#include "SnapshotUpdatePlan.h"

#include <cassert>
#include <iostream>

static void test_default_plan_has_no_refresh_work()
{
    SnapshotUpdatePlan plan;

    assert(plan.hasRefreshWork() == false);
    assert(plan.requiresFullSnapshot() == false);
    assert(plan.shouldRefreshStatus() == false);
    assert(plan.shouldRefreshChannels() == false);
    assert(plan.shouldRefreshRecordings() == false);
    assert(plan.shouldRefreshTimers() == false);
    assert(plan.shouldRefreshEvents() == false);
}

static void test_domain_refresh_flags_are_independent()
{
    SnapshotUpdatePlan plan;

    plan.markChannelsRefresh();
    plan.markRecordingsRefresh();

    assert(plan.hasRefreshWork() == true);
    assert(plan.requiresFullSnapshot() == false);
    assert(plan.shouldRefreshStatus() == false);
    assert(plan.shouldRefreshChannels() == true);
    assert(plan.shouldRefreshRecordings() == true);
    assert(plan.shouldRefreshTimers() == false);
    assert(plan.shouldRefreshEvents() == false);
}

static void test_full_snapshot_refresh_implies_all_domains()
{
    SnapshotUpdatePlan plan;

    plan.markFullSnapshotRefresh();

    assert(plan.hasRefreshWork() == true);
    assert(plan.requiresFullSnapshot() == true);
    assert(plan.shouldRefreshStatus() == true);
    assert(plan.shouldRefreshChannels() == true);
    assert(plan.shouldRefreshRecordings() == true);
    assert(plan.shouldRefreshTimers() == true);
    assert(plan.shouldRefreshEvents() == true);
}

int main()
{
    test_default_plan_has_no_refresh_work();
    test_domain_refresh_flags_are_independent();
    test_full_snapshot_refresh_implies_all_domains();

    std::cout
        << "test_snapshot_update_plan passed"
        << std::endl;

    return 0;
}
