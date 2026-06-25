#include "SearchTimerAutomationCandidateTimerProposal.h"
#include "SearchTimerAutomationDuplicateDetection.h"
#include "SearchTimerAutomationEvaluationPlan.h"
#include "SearchTimerAutomationMatchCandidate.h"
#include "SearchTimerAutomationReadOnlyService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{

bool contains(
    const std::vector<std::string>& values,
    const std::string& expected)
{
    for (const auto& value : values)
    {
        if (value == expected)
        {
            return true;
        }
    }

    return false;
}

} // namespace

int main()
{
    SearchTimerAutomationEvaluationPlan plan =
        SearchTimerAutomationEvaluationPlan::createReadOnly("default");
    plan.setCandidateLimit(10);

    SearchTimerAutomationMatchCandidate candidate =
        SearchTimerAutomationMatchCandidate::createReadOnly(
            "default",
            "search-1",
            "event-1");

    candidate.setSearchTimerName("Movies");
    candidate.setEventTitle("The Village - Das Dorf");
    candidate.setChannelId("C-1-1107-898");
    candidate.setEventStartTime(1288488540);
    candidate.setEventDuration(5820);
    candidate.setMatchScore(87);
    candidate.addMatchReason("title contains search phrase");

    SearchTimerAutomationDuplicateDetection duplicateDetection =
        SearchTimerAutomationDuplicateDetection::forCandidate(candidate);
    duplicateDetection.setRiskLevel(
        SearchTimerAutomationDuplicateRiskLevel::High);
    duplicateDetection.setExistingTimerId("timer-1");
    duplicateDetection.addDuplicateReason("existing timer overlaps event");

    SearchTimerAutomationCandidateTimerProposal proposal =
        SearchTimerAutomationCandidateTimerProposal
            ::fromCandidateAndDuplicateDetection(
                candidate,
                duplicateDetection);

    SearchTimerAutomationReadOnlyService service;

    SearchTimerAutomationDryRunResult result =
        service.evaluate(
            plan,
            std::vector<SearchTimerAutomationMatchCandidate>{candidate},
            std::vector<SearchTimerAutomationDuplicateDetection>{duplicateDetection},
            std::vector<SearchTimerAutomationCandidateTimerProposal>{proposal},
            3);

    assert(result.backendId() == "default");
    assert(result.candidateLimit() == 10);
    assert(result.evaluatedSearchTimerCount() == 3);
    assert(result.matchedCandidateCount() == 1);
    assert(result.duplicateRiskCount() == 1);
    assert(result.proposalCount() == 1);
    assert(result.allowedProposalCount() == 0);
    assert(result.blockedProposalCount() == 1);
    assert(result.hasContent());
    assert(result.successful());
    assert(result.isValid());
    assert(result.validationErrors().empty());
    assert(result.dryRunOnly());
    assert(!result.mutationAllowed());
    assert(!result.timerCreationAllowed());
    assert(!result.backendWriteAllowed());
    assert(!result.automaticExecutionAllowed());
    assert(result.requiresExplicitExecutionHandoff());
    assert(!result.hasErrors());
    assert(!result.hasWarnings());
    assert(contains(
        result.auditTrail(),
        "read-only automation service completed without mutation"));

    SearchTimerAutomationDryRunResult mismatchedResult =
        service.evaluate(
            plan,
            std::vector<SearchTimerAutomationMatchCandidate>{candidate},
            std::vector<SearchTimerAutomationDuplicateDetection>{},
            std::vector<SearchTimerAutomationCandidateTimerProposal>{},
            -5);

    assert(mismatchedResult.evaluatedSearchTimerCount() == 0);
    assert(mismatchedResult.hasWarnings());
    assert(contains(
        mismatchedResult.warnings(),
        "evaluated search timer count was clamped to zero"));
    assert(contains(
        mismatchedResult.warnings(),
        "duplicate detection count differs from match candidate count"));
    assert(contains(
        mismatchedResult.warnings(),
        "candidate timer proposal count differs from match candidate count"));
    assert(mismatchedResult.dryRunOnly());
    assert(!mismatchedResult.mutationAllowed());
    assert(!mismatchedResult.timerCreationAllowed());

    SearchTimerAutomationEvaluationPlan invalidPlan =
        SearchTimerAutomationEvaluationPlan::createReadOnly("");

    SearchTimerAutomationDryRunResult invalidResult =
        service.evaluate(
            invalidPlan,
            std::vector<SearchTimerAutomationMatchCandidate>{candidate},
            std::vector<SearchTimerAutomationDuplicateDetection>{duplicateDetection},
            std::vector<SearchTimerAutomationCandidateTimerProposal>{proposal},
            1);

    assert(!invalidResult.isValid());
    assert(!invalidResult.successful());
    assert(invalidResult.hasErrors());
    assert(contains(
        invalidResult.errors(),
        "backend id is required"));
    assert(!invalidResult.hasContent());
    assert(invalidResult.matchedCandidateCount() == 0);
    assert(invalidResult.duplicateRiskCount() == 0);
    assert(invalidResult.proposalCount() == 0);
    assert(invalidResult.dryRunOnly());
    assert(!invalidResult.mutationAllowed());
    assert(!invalidResult.backendWriteAllowed());
    assert(!invalidResult.automaticExecutionAllowed());
    assert(contains(
        invalidResult.auditTrail(),
        "read-only automation service rejected invalid plan"));

    std::cout
        << "test_search_timer_automation_read_only_service passed"
        << std::endl;

    return 0;
}
