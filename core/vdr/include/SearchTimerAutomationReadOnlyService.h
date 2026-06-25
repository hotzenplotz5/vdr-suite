#pragma once

#include "SearchTimerAutomationCandidateTimerProposal.h"
#include "SearchTimerAutomationDryRunResult.h"
#include "SearchTimerAutomationDuplicateDetection.h"
#include "SearchTimerAutomationEvaluationPlan.h"
#include "SearchTimerAutomationMatchCandidate.h"

#include <vector>

class SearchTimerAutomationReadOnlyService
{
public:
    SearchTimerAutomationDryRunResult evaluate(
        const SearchTimerAutomationEvaluationPlan& plan,
        const std::vector<SearchTimerAutomationMatchCandidate>& matchCandidates,
        const std::vector<SearchTimerAutomationDuplicateDetection>& duplicateDetections,
        const std::vector<SearchTimerAutomationCandidateTimerProposal>& candidateTimerProposals,
        int evaluatedSearchTimerCount) const;
};
