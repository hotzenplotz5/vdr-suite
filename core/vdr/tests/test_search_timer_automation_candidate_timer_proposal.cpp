#include "SearchTimerAutomationCandidateTimerProposal.h"
#include "SearchTimerAutomationDuplicateDetection.h"
#include "SearchTimerAutomationMatchCandidate.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
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

    SearchTimerAutomationDuplicateDetection duplicateDetection =
        SearchTimerAutomationDuplicateDetection::forCandidate(candidate);

    SearchTimerAutomationCandidateTimerProposal proposal =
        SearchTimerAutomationCandidateTimerProposal
            ::fromCandidateAndDuplicateDetection(
                candidate,
                duplicateDetection);

    assert(proposal.backendId() == "default");
    assert(proposal.searchTimerId() == "search-1");
    assert(proposal.searchTimerName() == "Movies");
    assert(proposal.eventId() == "event-1");
    assert(proposal.eventTitle() == "The Village - Das Dorf");
    assert(proposal.channelId() == "C-1-1107-898");
    assert(proposal.eventStartTime() == 1288488540);
    assert(proposal.eventDuration() == 5820);
    assert(proposal.proposedStartTime() == 1288488540);
    assert(proposal.proposedEndTime() == 1288494360);
    assert(proposal.duplicateRiskName() == "none");
    assert(!proposal.requiresOperatorReview());
    assert(!proposal.hasExistingTimer());
    assert(!proposal.hasExistingRecording());
    assert(!proposal.blocked());
    assert(proposal.proposalAllowed());
    assert(proposal.dryRunOnly());
    assert(!proposal.mutationAllowed());
    assert(!proposal.timerCreationAllowed());
    assert(!proposal.backendWriteAllowed());
    assert(!proposal.automaticExecutionAllowed());
    assert(proposal.requiresExplicitExecutionHandoff());
    assert(proposal.isValid());
    assert(proposal.validationErrors().empty());

    proposal.applyMargins(5, 10);
    assert(proposal.startMarginMinutes() == 5);
    assert(proposal.stopMarginMinutes() == 10);
    assert(proposal.proposedStartTime() == 1288488240);
    assert(proposal.proposedEndTime() == 1288494960);

    proposal.applyMargins(-1, -5);
    assert(proposal.startMarginMinutes() == 0);
    assert(proposal.stopMarginMinutes() == 0);
    assert(proposal.proposedStartTime() == 1288488540);
    assert(proposal.proposedEndTime() == 1288494360);

    proposal.setRecordingDirectory("Mystery");
    proposal.setPriority(85);
    proposal.setLifetime(30);
    proposal.addProposalReason("candidate can be represented as timer preview");
    proposal.addProposalReason("");

    assert(proposal.recordingDirectory() == "Mystery");
    assert(proposal.priority() == 85);
    assert(proposal.lifetime() == 30);
    assert(proposal.hasProposalReasons());
    assert(proposal.proposalReasons().size() == 1);

    proposal.setPriority(-1);
    assert(proposal.priority() == 0);

    proposal.setPriority(200);
    assert(proposal.priority() == 99);

    proposal.setLifetime(-1);
    assert(proposal.lifetime() == 0);

    proposal.setLifetime(200);
    assert(proposal.lifetime() == 99);

    proposal.markBlocked("manual safety review required");
    assert(proposal.blocked());
    assert(!proposal.proposalAllowed());
    assert(proposal.hasBlockReasons());
    assert(proposal.blockReasons().front() == "manual safety review required");

    SearchTimerAutomationMatchCandidate riskyCandidate =
        SearchTimerAutomationMatchCandidate::createReadOnly(
            "default",
            "search-1",
            "event-1");

    riskyCandidate.setEventTitle("The Village - Das Dorf");
    riskyCandidate.setChannelId("C-1-1107-898");
    riskyCandidate.setEventStartTime(1288488540);
    riskyCandidate.setEventDuration(5820);

    SearchTimerAutomationDuplicateDetection riskyDuplicateDetection =
        SearchTimerAutomationDuplicateDetection::forCandidate(riskyCandidate);

    riskyDuplicateDetection.setRiskLevel(
        SearchTimerAutomationDuplicateRiskLevel::High);
    riskyDuplicateDetection.setExistingTimerId("timer-1");
    riskyDuplicateDetection.setExistingRecordingPath(
        "/srv/vdr/video/Mystery/Existing.rec");

    SearchTimerAutomationCandidateTimerProposal riskyProposal =
        SearchTimerAutomationCandidateTimerProposal
            ::fromCandidateAndDuplicateDetection(
                riskyCandidate,
                riskyDuplicateDetection);

    assert(riskyProposal.duplicateRiskName() == "high");
    assert(riskyProposal.requiresOperatorReview());
    assert(riskyProposal.hasExistingTimer());
    assert(riskyProposal.existingTimerId() == "timer-1");
    assert(riskyProposal.hasExistingRecording());
    assert(riskyProposal.existingRecordingPath()
        == "/srv/vdr/video/Mystery/Existing.rec");
    assert(riskyProposal.blocked());
    assert(!riskyProposal.proposalAllowed());
    assert(riskyProposal.hasBlockReasons());
    assert(!riskyProposal.mutationAllowed());
    assert(!riskyProposal.timerCreationAllowed());
    assert(!riskyProposal.automaticExecutionAllowed());

    SearchTimerAutomationMatchCandidate invalidCandidate =
        SearchTimerAutomationMatchCandidate::createReadOnly("", "", "");

    SearchTimerAutomationDuplicateDetection invalidDuplicateDetection =
        SearchTimerAutomationDuplicateDetection::forCandidate(invalidCandidate);

    SearchTimerAutomationCandidateTimerProposal invalidProposal =
        SearchTimerAutomationCandidateTimerProposal
            ::fromCandidateAndDuplicateDetection(
                invalidCandidate,
                invalidDuplicateDetection);

    assert(!invalidProposal.isValid());

    const std::vector<std::string> errors =
        invalidProposal.validationErrors();

    assert(errors.size() == 5);
    assert(errors[0] == "backend id is required");
    assert(errors[1] == "search timer id is required");
    assert(errors[2] == "event id is required");
    assert(errors[3] == "channel id is required");
    assert(errors[4] == "proposal end time must be after start time");
    assert(invalidProposal.dryRunOnly());
    assert(!invalidProposal.mutationAllowed());
    assert(!invalidProposal.timerCreationAllowed());
    assert(!invalidProposal.backendWriteAllowed());

    std::cout
        << "test_search_timer_automation_candidate_timer_proposal passed"
        << std::endl;

    return 0;
}
