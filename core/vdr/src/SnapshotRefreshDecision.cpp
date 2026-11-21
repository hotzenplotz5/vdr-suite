#include "SnapshotRefreshDecision.h"

SnapshotRefreshDecision::SnapshotRefreshDecision(SnapshotRefreshDecisionType type)
    : type_(type)
{
}

SnapshotRefreshDecisionType SnapshotRefreshDecision::type() const
{
    return type_;
}

bool SnapshotRefreshDecision::shouldRefreshSnapshot() const
{
    return type_ == SnapshotRefreshDecisionType::FullRefresh;
}

SnapshotRefreshDecision SnapshotRefreshDecision::noRefresh()
{
    return SnapshotRefreshDecision(SnapshotRefreshDecisionType::NoRefresh);
}

SnapshotRefreshDecision SnapshotRefreshDecision::fullRefresh()
{
    return SnapshotRefreshDecision(SnapshotRefreshDecisionType::FullRefresh);
}
