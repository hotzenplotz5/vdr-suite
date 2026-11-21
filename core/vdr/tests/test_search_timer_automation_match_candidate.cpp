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

    assert(candidate.backendId() == "default");
    assert(candidate.searchTimerId() == "search-1");
    assert(candidate.eventId() == "event-1");
    assert(candidate.dryRunOnly());
    assert(!candidate.mutationAllowed());
    assert(!candidate.timerProposalCreated());
    assert(candidate.requiresDuplicateCheck());
    assert(candidate.isValid());
    assert(candidate.validationErrors().empty());

    candidate.setSearchTimerName("Movies");
    candidate.setEventTitle("The Village - Das Dorf");
    candidate.setChannelId("C-1-1107-898");
    candidate.setEventStartTime(1288488540);
    candidate.setEventDuration(5820);
    candidate.setMatchScore(87);
    candidate.addMatchReason("title contains search phrase");
    candidate.addMatchReason("");

    assert(candidate.searchTimerName() == "Movies");
    assert(candidate.eventTitle() == "The Village - Das Dorf");
    assert(candidate.channelId() == "C-1-1107-898");
    assert(candidate.eventStartTime() == 1288488540);
    assert(candidate.eventDuration() == 5820);
    assert(candidate.matchScore() == 87);
    assert(candidate.hasMatchReasons());
    assert(candidate.matchReasons().size() == 1);
    assert(candidate.matchReasons().front() == "title contains search phrase");

    candidate.setMatchScore(-5);
    assert(candidate.matchScore() == 0);

    candidate.setMatchScore(200);
    assert(candidate.matchScore() == 100);

    candidate.setEventStartTime(-10);
    assert(candidate.eventStartTime() == 0);

    candidate.setEventDuration(-1);
    assert(candidate.eventDuration() == 0);

    SearchTimerAutomationMatchCandidate invalidCandidate =
        SearchTimerAutomationMatchCandidate::createReadOnly("", "", "");

    assert(!invalidCandidate.isValid());

    const std::vector<std::string> errors =
        invalidCandidate.validationErrors();

    assert(errors.size() == 3);
    assert(errors[0] == "backend id is required");
    assert(errors[1] == "search timer id is required");
    assert(errors[2] == "event id is required");
    assert(invalidCandidate.dryRunOnly());
    assert(!invalidCandidate.mutationAllowed());
    assert(!invalidCandidate.timerProposalCreated());

    std::cout
        << "test_search_timer_automation_match_candidate passed"
        << std::endl;

    return 0;
}
