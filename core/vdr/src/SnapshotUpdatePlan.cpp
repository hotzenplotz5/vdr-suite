#include "SnapshotUpdatePlan.h"

SnapshotUpdatePlan::SnapshotUpdatePlan()
    : refreshStatus_(false),
      refreshChannels_(false),
      refreshRecordings_(false),
      refreshTimers_(false),
      refreshSearchTimers_(false),
      refreshEvents_(false),
      selectiveEventRefresh_(false),
      selectiveEventQuery_(),
      fullSnapshotRefresh_(false)
{
}

bool SnapshotUpdatePlan::shouldRefreshStatus() const
{
    return refreshStatus_ || fullSnapshotRefresh_;
}

bool SnapshotUpdatePlan::shouldRefreshChannels() const
{
    return refreshChannels_ || fullSnapshotRefresh_;
}

bool SnapshotUpdatePlan::shouldRefreshRecordings() const
{
    return refreshRecordings_ || fullSnapshotRefresh_;
}

bool SnapshotUpdatePlan::shouldRefreshTimers() const
{
    return refreshTimers_ || fullSnapshotRefresh_;
}

bool SnapshotUpdatePlan::shouldRefreshSearchTimers() const
{
    return refreshSearchTimers_ || fullSnapshotRefresh_;
}

bool SnapshotUpdatePlan::shouldRefreshEvents() const
{
    return refreshEvents_ || fullSnapshotRefresh_;
}

bool SnapshotUpdatePlan::hasSelectiveEventRefresh() const
{
    return selectiveEventRefresh_;
}

VdrEventQuery SnapshotUpdatePlan::selectiveEventQuery() const
{
    return selectiveEventQuery_;
}

bool SnapshotUpdatePlan::requiresFullSnapshot() const
{
    return fullSnapshotRefresh_;
}

bool SnapshotUpdatePlan::hasRefreshWork() const
{
    return shouldRefreshStatus()
        || shouldRefreshChannels()
        || shouldRefreshRecordings()
        || shouldRefreshTimers()
        || shouldRefreshSearchTimers()
        || shouldRefreshEvents()
        || hasSelectiveEventRefresh();
}

void SnapshotUpdatePlan::markStatusRefresh()
{
    refreshStatus_ = true;
}

void SnapshotUpdatePlan::markChannelsRefresh()
{
    refreshChannels_ = true;
}

void SnapshotUpdatePlan::markRecordingsRefresh()
{
    refreshRecordings_ = true;
}

void SnapshotUpdatePlan::markTimersRefresh()
{
    refreshTimers_ = true;
}

void SnapshotUpdatePlan::markSearchTimersRefresh()
{
    refreshSearchTimers_ = true;
}

void SnapshotUpdatePlan::markEventsRefresh()
{
    refreshEvents_ = true;
}

void SnapshotUpdatePlan::markSelectiveEventRefresh(const VdrEventQuery& query)
{
    selectiveEventRefresh_ = true;
    selectiveEventQuery_ = query;
}

void SnapshotUpdatePlan::markFullSnapshotRefresh()
{
    fullSnapshotRefresh_ = true;
}
