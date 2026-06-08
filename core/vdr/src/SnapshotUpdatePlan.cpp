#include "SnapshotUpdatePlan.h"

SnapshotUpdatePlan::SnapshotUpdatePlan()
    : refreshStatus_(false),
      refreshChannels_(false),
      refreshRecordings_(false),
      refreshTimers_(false),
      refreshEvents_(false),
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

bool SnapshotUpdatePlan::shouldRefreshEvents() const
{
    return refreshEvents_ || fullSnapshotRefresh_;
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
        || shouldRefreshEvents();
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

void SnapshotUpdatePlan::markEventsRefresh()
{
    refreshEvents_ = true;
}

void SnapshotUpdatePlan::markFullSnapshotRefresh()
{
    fullSnapshotRefresh_ = true;
}
