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
    assert(plan.hasSelectiveEventRefresh() == false);
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
    assert(plan.hasSelectiveEventRefresh() == false);
}

static void test_selective_event_refresh_stores_query_without_full_event_refresh()
{
    SnapshotUpdatePlan plan;

    VdrEventQuery query;
    query.channelId = "channel-1";
    query.from = 1700000000;
    query.timespan = 3600;
    query.limit = 50;

    plan.markSelectiveEventRefresh(query);

    const auto storedQuery = plan.selectiveEventQuery();

    assert(plan.hasRefreshWork() == true);
    assert(plan.requiresFullSnapshot() == false);
    assert(plan.shouldRefreshEvents() == false);
    assert(plan.hasSelectiveEventRefresh() == true);
    assert(storedQuery.channelId == "channel-1");
    assert(storedQuery.from == 1700000000);
    assert(storedQuery.timespan == 3600);
    assert(storedQuery.limit == 50);
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
    assert(plan.hasSelectiveEventRefresh() == false);
}

int main()
{
    test_default_plan_has_no_refresh_work();
    test_domain_refresh_flags_are_independent();
    test_selective_event_refresh_stores_query_without_full_event_refresh();
    test_full_snapshot_refresh_implies_all_domains();

    std::cout
        << "test_snapshot_update_plan passed"
        << std::endl;

    return 0;
}
