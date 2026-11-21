#pragma once

#include "SearchTimerAutomationCandidateTimerProposal.h"
#include "SearchTimerAutomationDuplicateDetection.h"
#include "SearchTimerAutomationEvaluationPlan.h"
#include "SearchTimerAutomationMatchCandidate.h"

#include <string>
#include <vector>

class SearchTimerAutomationDryRunResult
{
public:
    static SearchTimerAutomationDryRunResult forPlan(
        const SearchTimerAutomationEvaluationPlan& plan)
    {
        SearchTimerAutomationDryRunResult result;
        result.backendId_ = plan.backendId();
        result.candidateLimit_ = plan.candidateLimit();
        result.includeInactiveSearchTimers_ =
            plan.includeInactiveSearchTimers();
        result.includeExistingTimers_ = plan.includeExistingTimers();
        result.includeRecordings_ = plan.includeRecordings();
        result.addAuditEntry("dry-run result created");
        return result;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    int candidateLimit() const
    {
        return candidateLimit_;
    }

    bool includeInactiveSearchTimers() const
    {
        return includeInactiveSearchTimers_;
    }

    bool includeExistingTimers() const
    {
        return includeExistingTimers_;
    }

    bool includeRecordings() const
    {
        return includeRecordings_;
    }

    bool dryRunOnly() const
    {
        return true;
    }

    bool mutationAllowed() const
    {
        return false;
    }

    bool timerCreationAllowed() const
    {
        return false;
    }

    bool backendWriteAllowed() const
    {
        return false;
    }

    bool automaticExecutionAllowed() const
    {
        return false;
    }

    bool requiresExplicitExecutionHandoff() const
    {
        return true;
    }

    void setEvaluatedSearchTimerCount(
        int evaluatedSearchTimerCount)
    {
        evaluatedSearchTimerCount_ = evaluatedSearchTimerCount < 0
            ? 0
            : evaluatedSearchTimerCount;
    }

    int evaluatedSearchTimerCount() const
    {
        return evaluatedSearchTimerCount_;
    }

    void addMatchCandidate(
        const SearchTimerAutomationMatchCandidate& candidate)
    {
        matchCandidates_.push_back(candidate);
    }

    const std::vector<SearchTimerAutomationMatchCandidate>& matchCandidates() const
    {
        return matchCandidates_;
    }

    void addDuplicateDetection(
        const SearchTimerAutomationDuplicateDetection& duplicateDetection)
    {
        duplicateDetections_.push_back(duplicateDetection);
    }

    const std::vector<SearchTimerAutomationDuplicateDetection>& duplicateDetections() const
    {
        return duplicateDetections_;
    }

    void addCandidateTimerProposal(
        const SearchTimerAutomationCandidateTimerProposal& proposal)
    {
        candidateTimerProposals_.push_back(proposal);
    }

    const std::vector<SearchTimerAutomationCandidateTimerProposal>& candidateTimerProposals() const
    {
        return candidateTimerProposals_;
    }

    int matchedCandidateCount() const
    {
        return static_cast<int>(matchCandidates_.size());
    }

    int duplicateRiskCount() const
    {
        int count = 0;

        for (const auto& duplicateDetection : duplicateDetections_)
        {
            if (duplicateDetection.hasRisk())
            {
                ++count;
            }
        }

        return count;
    }

    int proposalCount() const
    {
        return static_cast<int>(candidateTimerProposals_.size());
    }

    int blockedProposalCount() const
    {
        int count = 0;

        for (const auto& proposal : candidateTimerProposals_)
        {
            if (proposal.blocked())
            {
                ++count;
            }
        }

        return count;
    }

    int allowedProposalCount() const
    {
        int count = 0;

        for (const auto& proposal : candidateTimerProposals_)
        {
            if (proposal.proposalAllowed())
            {
                ++count;
            }
        }

        return count;
    }

    void addWarning(
        const std::string& warning)
    {
        if (!warning.empty())
        {
            warnings_.push_back(warning);
        }
    }

    const std::vector<std::string>& warnings() const
    {
        return warnings_;
    }

    bool hasWarnings() const
    {
        return !warnings_.empty();
    }

    void addError(
        const std::string& error)
    {
        if (!error.empty())
        {
            errors_.push_back(error);
        }
    }

    const std::vector<std::string>& errors() const
    {
        return errors_;
    }

    bool hasErrors() const
    {
        return !errors_.empty();
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

    bool successful() const
    {
        return !hasErrors();
    }

    bool hasContent() const
    {
        return !matchCandidates_.empty()
            || !duplicateDetections_.empty()
            || !candidateTimerProposals_.empty();
    }

    bool isValid() const
    {
        return !backendId_.empty()
            && candidateLimit_ > 0;
    }

    std::vector<std::string> validationErrors() const
    {
        std::vector<std::string> validationErrors;

        if (backendId_.empty())
        {
            validationErrors.push_back("backend id is required");
        }

        if (candidateLimit_ <= 0)
        {
            validationErrors.push_back("candidate limit must be positive");
        }

        return validationErrors;
    }

private:
    std::string backendId_;
    int candidateLimit_ = 0;
    bool includeInactiveSearchTimers_ = false;
    bool includeExistingTimers_ = true;
    bool includeRecordings_ = true;
    int evaluatedSearchTimerCount_ = 0;
    std::vector<SearchTimerAutomationMatchCandidate> matchCandidates_;
    std::vector<SearchTimerAutomationDuplicateDetection> duplicateDetections_;
    std::vector<SearchTimerAutomationCandidateTimerProposal> candidateTimerProposals_;
    std::vector<std::string> warnings_;
    std::vector<std::string> errors_;
    std::vector<std::string> auditTrail_;
};
