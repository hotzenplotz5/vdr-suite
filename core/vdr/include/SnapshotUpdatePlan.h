#ifndef SNAPSHOT_UPDATE_PLAN_H
#define SNAPSHOT_UPDATE_PLAN_H

#include "VdrEventQuery.h"

class SnapshotUpdatePlan {
public:
    SnapshotUpdatePlan();

    bool shouldRefreshStatus() const;
    bool shouldRefreshChannels() const;
    bool shouldRefreshRecordings() const;
    bool shouldRefreshTimers() const;
    bool shouldRefreshSearchTimers() const;
    bool shouldRefreshEvents() const;
    bool hasSelectiveEventRefresh() const;
    VdrEventQuery selectiveEventQuery() const;
    bool requiresFullSnapshot() const;
    bool hasRefreshWork() const;

    void markStatusRefresh();
    void markChannelsRefresh();
    void markRecordingsRefresh();
    void markTimersRefresh();
    void markSearchTimersRefresh();
    void markEventsRefresh();
    void markSelectiveEventRefresh(const VdrEventQuery& query);
    void markFullSnapshotRefresh();

private:
    bool refreshStatus_;
    bool refreshChannels_;
    bool refreshRecordings_;
    bool refreshTimers_;
    bool refreshSearchTimers_;
    bool refreshEvents_;
    bool selectiveEventRefresh_;
    VdrEventQuery selectiveEventQuery_;
    bool fullSnapshotRefresh_;
};

#endif
