#include "SearchTimerAutomationReadOnlyService.h"

#include <string>
#include <vector>

namespace
{

void appendValidationErrors(
    SearchTimerAutomationDryRunResult& result,
    const std::vector<std::string>& validationErrors)
{
    for (const auto& validationError : validationErrors)
    {
        result.addError(validationError);
    }
}

} // namespace

SearchTimerAutomationDryRunResult SearchTimerAutomationReadOnlyService::evaluate(
    const SearchTimerAutomationEvaluationPlan& plan,
    const std::vector<SearchTimerAutomationMatchCandidate>& matchCandidates,
    const std::vector<SearchTimerAutomationDuplicateDetection>& duplicateDetections,
    const std::vector<SearchTimerAutomationCandidateTimerProposal>& candidateTimerProposals,
    int evaluatedSearchTimerCount) const
{
    SearchTimerAutomationDryRunResult result =
        SearchTimerAutomationDryRunResult::forPlan(plan);

    result.addAuditEntry("read-only automation service boundary entered");
    result.setEvaluatedSearchTimerCount(evaluatedSearchTimerCount);

    if (!plan.isValid())
    {
        appendValidationErrors(result, plan.validationErrors());
        result.addAuditEntry("read-only automation service rejected invalid plan");
        return result;
    }

    if (evaluatedSearchTimerCount < 0)
    {
        result.addWarning("evaluated search timer count was clamped to zero");
    }

    if (static_cast<int>(matchCandidates.size()) > plan.candidateLimit())
    {
        result.addWarning("match candidate count exceeds plan candidate limit");
    }

    if (duplicateDetections.size() != matchCandidates.size())
    {
        result.addWarning("duplicate detection count differs from match candidate count");
    }

    if (candidateTimerProposals.size() != matchCandidates.size())
    {
        result.addWarning("candidate timer proposal count differs from match candidate count");
    }

    for (const auto& matchCandidate : matchCandidates)
    {
        result.addMatchCandidate(matchCandidate);
    }

    for (const auto& duplicateDetection : duplicateDetections)
    {
        result.addDuplicateDetection(duplicateDetection);
    }

    for (const auto& candidateTimerProposal : candidateTimerProposals)
    {
        result.addCandidateTimerProposal(candidateTimerProposal);
    }

    result.addAuditEntry("read-only automation service aggregated candidates");
    result.addAuditEntry("read-only automation service completed without mutation");

    return result;
}
