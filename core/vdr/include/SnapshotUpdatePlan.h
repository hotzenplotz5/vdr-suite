#ifndef SNAPSHOT_UPDATE_PLAN_H
#define SNAPSHOT_UPDATE_PLAN_H

class SnapshotUpdatePlan {
public:
    SnapshotUpdatePlan();

    bool shouldRefreshStatus() const;
    bool shouldRefreshChannels() const;
    bool shouldRefreshRecordings() const;
    bool shouldRefreshTimers() const;
    bool shouldRefreshEvents() const;
    bool requiresFullSnapshot() const;
    bool hasRefreshWork() const;

    void markStatusRefresh();
    void markChannelsRefresh();
    void markRecordingsRefresh();
    void markTimersRefresh();
    void markEventsRefresh();
    void markFullSnapshotRefresh();

private:
    bool refreshStatus_;
    bool refreshChannels_;
    bool refreshRecordings_;
    bool refreshTimers_;
    bool refreshEvents_;
    bool fullSnapshotRefresh_;
};

#endif
