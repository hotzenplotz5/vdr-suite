#include "SearchTimerAutomationCandidateTimerProposal.h"
#include "SearchTimerAutomationDuplicateDetection.h"
#include "SearchTimerAutomationDryRunResult.h"
#include "SearchTimerAutomationDryRunResultJsonSerializer.h"
#include "SearchTimerAutomationEvaluationPlan.h"
#include "SearchTimerAutomationMatchCandidate.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{

bool contains(
    const std::string& haystack,
    const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

} // namespace

int main()
{
    SearchTimerAutomationEvaluationPlan plan =
        SearchTimerAutomationEvaluationPlan::createReadOnly("default");
    plan.setCandidateLimit(25);
    plan.setIncludeInactiveSearchTimers(true);

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
    duplicateDetection.setExistingRecordingPath(
        "/srv/vdr/video/Mystery/Existing.rec");
    duplicateDetection.setTitleSimilarity(91);
    duplicateDetection.setTimeOverlapSeconds(5400);
    duplicateDetection.addDuplicateReason("existing timer overlaps event");

    SearchTimerAutomationCandidateTimerProposal proposal =
        SearchTimerAutomationCandidateTimerProposal
            ::fromCandidateAndDuplicateDetection(
                candidate,
                duplicateDetection);
    proposal.applyMargins(5, 10);
    proposal.setRecordingDirectory("Mystery");
    proposal.setPriority(85);
    proposal.setLifetime(30);
    proposal.addProposalReason("candidate can be represented as timer preview");

    SearchTimerAutomationDryRunResult result =
        SearchTimerAutomationDryRunResult::forPlan(plan);
    result.setEvaluatedSearchTimerCount(3);
    result.addMatchCandidate(candidate);
    result.addDuplicateDetection(duplicateDetection);
    result.addCandidateTimerProposal(proposal);
    result.addWarning("dry-run warning");
    result.addAuditEntry("candidate serialized");

    assert(result.backendId() == "default");
    assert(result.candidateLimit() == 25);
    assert(result.includeInactiveSearchTimers());
    assert(result.includeExistingTimers());
    assert(result.includeRecordings());
    assert(result.dryRunOnly());
    assert(!result.mutationAllowed());
    assert(!result.timerCreationAllowed());
    assert(!result.backendWriteAllowed());
    assert(!result.automaticExecutionAllowed());
    assert(result.requiresExplicitExecutionHandoff());
    assert(result.evaluatedSearchTimerCount() == 3);
    assert(result.matchedCandidateCount() == 1);
    assert(result.duplicateRiskCount() == 1);
    assert(result.proposalCount() == 1);
    assert(result.allowedProposalCount() == 0);
    assert(result.blockedProposalCount() == 1);
    assert(result.hasWarnings());
    assert(!result.hasErrors());
    assert(result.successful());
    assert(result.hasContent());
    assert(result.isValid());
    assert(result.validationErrors().empty());

    SearchTimerAutomationDryRunResultJsonSerializer serializer;
    const std::string json = serializer.serialize(result);

    assert(contains(json, "\"success\":true"));
    assert(contains(json, "\"dryRunOnly\":true"));
    assert(contains(json, "\"mutationAllowed\":false"));
    assert(contains(json, "\"timerCreationAllowed\":false"));
    assert(contains(json, "\"backendWriteAllowed\":false"));
    assert(contains(json, "\"automaticExecutionAllowed\":false"));
    assert(contains(json, "\"requiresExplicitExecutionHandoff\":true"));
    assert(contains(json, "\"backendId\":\"default\""));
    assert(contains(json, "\"candidateLimit\":25"));
    assert(contains(json, "\"includeInactiveSearchTimers\":true"));
    assert(contains(json, "\"evaluatedSearchTimerCount\":3"));
    assert(contains(json, "\"matchedCandidateCount\":1"));
    assert(contains(json, "\"duplicateRiskCount\":1"));
    assert(contains(json, "\"proposalCount\":1"));
    assert(contains(json, "\"allowedProposalCount\":0"));
    assert(contains(json, "\"blockedProposalCount\":1"));
    assert(contains(json, "\"matchCandidates\":["));
    assert(contains(json, "\"duplicateDetections\":["));
    assert(contains(json, "\"candidateTimerProposals\":["));
    assert(contains(json, "\"searchTimerName\":\"Movies\""));
    assert(contains(json, "\"eventTitle\":\"The Village - Das Dorf\""));
    assert(contains(json, "\"riskLevel\":\"high\""));
    assert(contains(json, "\"existingTimerId\":\"timer-1\""));
    assert(contains(json, "\"recordingDirectory\":\"Mystery\""));
    assert(contains(json, "\"blocked\":true"));
    assert(contains(json, "\"dry-run warning\""));
    assert(contains(json, "\"candidate serialized\""));

    SearchTimerAutomationEvaluationPlan invalidPlan =
        SearchTimerAutomationEvaluationPlan::createReadOnly("");

    SearchTimerAutomationDryRunResult invalidResult =
        SearchTimerAutomationDryRunResult::forPlan(invalidPlan);
    invalidResult.addError("invalid dry-run");

    assert(!invalidResult.isValid());
    assert(!invalidResult.successful());

    const std::string invalidJson = serializer.serialize(invalidResult);

    assert(contains(invalidJson, "\"success\":false"));
    assert(contains(invalidJson, "\"valid\":false"));
    assert(contains(invalidJson, "\"backend id is required\""));
    assert(contains(invalidJson, "\"invalid dry-run\""));
    assert(contains(invalidJson, "\"dryRunOnly\":true"));
    assert(contains(invalidJson, "\"mutationAllowed\":false"));

    std::cout
        << "test_search_timer_automation_dry_run_result_json_serializer passed"
        << std::endl;

    return 0;
}
