#pragma once

#include <string>
#include <vector>

class SearchTimerAutomationEvaluationPlan
{
public:
    static SearchTimerAutomationEvaluationPlan createReadOnly(
        const std::string& backendId)
    {
        SearchTimerAutomationEvaluationPlan plan;
        plan.backendId_ = backendId;
        return plan;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    bool dryRunOnly() const
    {
        return true;
    }

    bool mutationAllowed() const
    {
        return false;
    }

    bool scheduledExecutionAllowed() const
    {
        return false;
    }

    bool requiresExplicitExecutionHandoff() const
    {
        return true;
    }

    int candidateLimit() const
    {
        return candidateLimit_;
    }

    void setCandidateLimit(
        int candidateLimit)
    {
        candidateLimit_ = candidateLimit < 1
            ? 1
            : candidateLimit;
    }

    bool includeInactiveSearchTimers() const
    {
        return includeInactiveSearchTimers_;
    }

    void setIncludeInactiveSearchTimers(
        bool includeInactiveSearchTimers)
    {
        includeInactiveSearchTimers_ = includeInactiveSearchTimers;
    }

    bool includeExistingTimers() const
    {
        return includeExistingTimers_;
    }

    void setIncludeExistingTimers(
        bool includeExistingTimers)
    {
        includeExistingTimers_ = includeExistingTimers;
    }

    bool includeRecordings() const
    {
        return includeRecordings_;
    }

    void setIncludeRecordings(
        bool includeRecordings)
    {
        includeRecordings_ = includeRecordings;
    }

    bool isValid() const
    {
        return !backendId_.empty()
            && candidateLimit_ > 0;
    }

    std::vector<std::string> validationErrors() const
    {
        std::vector<std::string> errors;

        if (backendId_.empty())
        {
            errors.push_back("backend id is required");
        }

        if (candidateLimit_ <= 0)
        {
            errors.push_back("candidate limit must be positive");
        }

        return errors;
    }

private:
    std::string backendId_;
    int candidateLimit_ = 50;
    bool includeInactiveSearchTimers_ = false;
    bool includeExistingTimers_ = true;
    bool includeRecordings_ = true;
};
