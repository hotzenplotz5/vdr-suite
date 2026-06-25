#pragma once

#include <string>
#include <vector>

class SearchTimerAutomationDaemonSchedulingPlan
{
public:
    static SearchTimerAutomationDaemonSchedulingPlan disabled(
        const std::string& backendId)
    {
        SearchTimerAutomationDaemonSchedulingPlan plan;
        plan.backendId_ = backendId;
        plan.enabled_ = false;
        plan.addAuditEntry("daemon scheduling plan created disabled");
        return plan;
    }

    static SearchTimerAutomationDaemonSchedulingPlan previewOnly(
        const std::string& backendId,
        int intervalMinutes)
    {
        SearchTimerAutomationDaemonSchedulingPlan plan;
        plan.backendId_ = backendId;
        plan.enabled_ = true;
        plan.previewOnly_ = true;
        plan.setIntervalMinutes(intervalMinutes);
        plan.addAuditEntry("daemon scheduling plan created for preview only");
        return plan;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    bool enabled() const
    {
        return enabled_;
    }

    bool previewOnly() const
    {
        return previewOnly_;
    }

    bool schedulerRuntimeAllowed() const
    {
        return false;
    }

    bool automaticExecutionAllowed() const
    {
        return false;
    }

    bool backendWriteAllowed() const
    {
        return false;
    }

    bool timerCreationAllowed() const
    {
        return false;
    }

    bool mutationAllowed() const
    {
        return false;
    }

    bool dryRunOnly() const
    {
        return true;
    }

    bool requiresExplicitExecutionHandoff() const
    {
        return true;
    }

    int intervalMinutes() const
    {
        return intervalMinutes_;
    }

    void setIntervalMinutes(
        int intervalMinutes)
    {
        if (intervalMinutes < 1)
        {
            intervalMinutes_ = 1;
            return;
        }

        intervalMinutes_ = intervalMinutes;
    }

    int maximumCandidatesPerRun() const
    {
        return maximumCandidatesPerRun_;
    }

    void setMaximumCandidatesPerRun(
        int maximumCandidatesPerRun)
    {
        if (maximumCandidatesPerRun < 1)
        {
            maximumCandidatesPerRun_ = 1;
            return;
        }

        maximumCandidatesPerRun_ = maximumCandidatesPerRun;
    }

    bool requireFreshSnapshot() const
    {
        return requireFreshSnapshot_;
    }

    void setRequireFreshSnapshot(
        bool requireFreshSnapshot)
    {
        requireFreshSnapshot_ = requireFreshSnapshot;
    }

    int maximumSnapshotAgeSeconds() const
    {
        return maximumSnapshotAgeSeconds_;
    }

    void setMaximumSnapshotAgeSeconds(
        int maximumSnapshotAgeSeconds)
    {
        if (maximumSnapshotAgeSeconds < 0)
        {
            maximumSnapshotAgeSeconds_ = 0;
            return;
        }

        maximumSnapshotAgeSeconds_ = maximumSnapshotAgeSeconds;
    }

    bool requireOperatorReviewForDuplicates() const
    {
        return requireOperatorReviewForDuplicates_;
    }

    void setRequireOperatorReviewForDuplicates(
        bool requireOperatorReviewForDuplicates)
    {
        requireOperatorReviewForDuplicates_ =
            requireOperatorReviewForDuplicates;
    }

    bool requireOperatorReviewBeforeExecution() const
    {
        return true;
    }

    void addSafetyReason(
        const std::string& reason)
    {
        if (!reason.empty())
        {
            safetyReasons_.push_back(reason);
        }
    }

    const std::vector<std::string>& safetyReasons() const
    {
        return safetyReasons_;
    }

    bool hasSafetyReasons() const
    {
        return !safetyReasons_.empty();
    }

    void addAuditEntry(
        const std::string& auditEntry)
    {
        if (!auditEntry.empty())
        {
            auditTrail_.push_back(auditEntry);
        }
    }

    const std::vector<std::string>& auditTrail() const
    {
        return auditTrail_;
    }

    bool hasAuditTrail() const
    {
        return !auditTrail_.empty();
    }

    bool isValid() const
    {
        return !backendId_.empty()
            && intervalMinutes_ > 0
            && maximumCandidatesPerRun_ > 0;
    }

    std::vector<std::string> validationErrors() const
    {
        std::vector<std::string> errors;

        if (backendId_.empty())
        {
            errors.push_back("backend id is required");
        }

        if (intervalMinutes_ <= 0)
        {
            errors.push_back("interval minutes must be positive");
        }

        if (maximumCandidatesPerRun_ <= 0)
        {
            errors.push_back("maximum candidates per run must be positive");
        }

        return errors;
    }

    std::vector<std::string> safetyInvariants() const
    {
        std::vector<std::string> invariants;

        invariants.push_back("scheduler runtime is disabled");
        invariants.push_back("automatic execution is disabled");
        invariants.push_back("backend writes are disabled");
        invariants.push_back("timer creation is disabled");
        invariants.push_back("mutation is disabled");
        invariants.push_back("dry-run only is enforced");
        invariants.push_back("explicit execution handoff is required");

        return invariants;
    }

private:
    std::string backendId_;
    bool enabled_ = false;
    bool previewOnly_ = true;
    int intervalMinutes_ = 60;
    int maximumCandidatesPerRun_ = 50;
    bool requireFreshSnapshot_ = true;
    int maximumSnapshotAgeSeconds_ = 300;
    bool requireOperatorReviewForDuplicates_ = true;
    std::vector<std::string> safetyReasons_;
    std::vector<std::string> auditTrail_;
};
