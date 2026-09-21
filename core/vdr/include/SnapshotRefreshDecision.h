#ifndef SNAPSHOT_REFRESH_DECISION_H
#define SNAPSHOT_REFRESH_DECISION_H

enum class SnapshotRefreshDecisionType {
    NoRefresh,
    FullRefresh
};

class SnapshotRefreshDecision {
public:
    explicit SnapshotRefreshDecision(SnapshotRefreshDecisionType type);

    SnapshotRefreshDecisionType type() const;
    bool shouldRefreshSnapshot() const;

    static SnapshotRefreshDecision noRefresh();
    static SnapshotRefreshDecision fullRefresh();

private:
    SnapshotRefreshDecisionType type_;
};

#endif
