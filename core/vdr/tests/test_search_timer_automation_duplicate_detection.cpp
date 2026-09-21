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

    candidate.setEventTitle("The Village - Das Dorf");
    candidate.setChannelId("C-1-1107-898");
    candidate.setEventStartTime(1288488540);
    candidate.setEventDuration(5820);

    SearchTimerAutomationDuplicateDetection detection =
        SearchTimerAutomationDuplicateDetection::forCandidate(candidate);

    assert(detection.backendId() == "default");
    assert(detection.searchTimerId() == "search-1");
    assert(detection.eventId() == "event-1");
    assert(detection.eventTitle() == "The Village - Das Dorf");
    assert(detection.channelId() == "C-1-1107-898");
    assert(detection.eventStartTime() == 1288488540);
    assert(detection.eventDuration() == 5820);
    assert(detection.riskLevel() == SearchTimerAutomationDuplicateRiskLevel::None);
    assert(!detection.hasRisk());
    assert(!detection.requiresOperatorReview());
    assert(!detection.blocksAutomaticProposal());
    assert(!detection.hasExistingTimer());
    assert(!detection.hasExistingRecording());
    assert(!detection.hasDuplicateReasons());
    assert(detection.dryRunOnly());
    assert(!detection.mutationAllowed());
    assert(!detection.automaticDecisionAllowed());
    assert(!detection.timerProposalCreated());
    assert(detection.isValid());
    assert(detection.validationErrors().empty());

    detection.setRiskLevel(SearchTimerAutomationDuplicateRiskLevel::Low);
    assert(detection.hasRisk());
    assert(!detection.requiresOperatorReview());
    assert(!detection.blocksAutomaticProposal());
    assert(SearchTimerAutomationDuplicateDetection::riskLevelName(
        detection.riskLevel()) == "low");

    detection.setRiskLevel(SearchTimerAutomationDuplicateRiskLevel::Medium);
    assert(detection.requiresOperatorReview());
    assert(!detection.blocksAutomaticProposal());

    detection.setRiskLevel(SearchTimerAutomationDuplicateRiskLevel::High);
    assert(detection.requiresOperatorReview());
    assert(detection.blocksAutomaticProposal());

    detection.setRiskLevel(SearchTimerAutomationDuplicateRiskLevel::Blocking);
    assert(detection.requiresOperatorReview());
    assert(detection.blocksAutomaticProposal());
    assert(SearchTimerAutomationDuplicateDetection::riskLevelName(
        detection.riskLevel()) == "blocking");

    detection.setExistingTimerId("timer-1");
    detection.setExistingRecordingPath("/srv/vdr/video/Mystery/Existing.rec");
    detection.setTitleSimilarity(91);
    detection.setTimeOverlapSeconds(5400);
    detection.addDuplicateReason("existing timer overlaps event");
    detection.addDuplicateReason("");

    assert(detection.hasExistingTimer());
    assert(detection.existingTimerId() == "timer-1");
    assert(detection.hasExistingRecording());
    assert(detection.existingRecordingPath() == "/srv/vdr/video/Mystery/Existing.rec");
    assert(detection.titleSimilarity() == 91);
    assert(detection.timeOverlapSeconds() == 5400);
    assert(detection.hasDuplicateReasons());
    assert(detection.duplicateReasons().size() == 1);
    assert(detection.duplicateReasons().front() == "existing timer overlaps event");

    detection.setTitleSimilarity(-1);
    assert(detection.titleSimilarity() == 0);

    detection.setTitleSimilarity(200);
    assert(detection.titleSimilarity() == 100);

    detection.setTimeOverlapSeconds(-10);
    assert(detection.timeOverlapSeconds() == 0);

    SearchTimerAutomationMatchCandidate invalidCandidate =
        SearchTimerAutomationMatchCandidate::createReadOnly("", "", "");

    SearchTimerAutomationDuplicateDetection invalidDetection =
        SearchTimerAutomationDuplicateDetection::forCandidate(invalidCandidate);

    assert(!invalidDetection.isValid());

    const std::vector<std::string> errors =
        invalidDetection.validationErrors();

    assert(errors.size() == 3);
    assert(errors[0] == "backend id is required");
    assert(errors[1] == "search timer id is required");
    assert(errors[2] == "event id is required");
    assert(invalidDetection.dryRunOnly());
    assert(!invalidDetection.mutationAllowed());
    assert(!invalidDetection.automaticDecisionAllowed());

    std::cout
        << "test_search_timer_automation_duplicate_detection passed"
        << std::endl;

    return 0;
}
