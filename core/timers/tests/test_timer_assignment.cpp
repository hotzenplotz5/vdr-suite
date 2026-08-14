#include "TimerAssignment.h"

#include <cassert>
#include <string>

using namespace vdrsuite::timers;

namespace
{
TimerAssignment validSelectedAssignment()
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "timer-assignment:1";
    assignment.assignmentRevision = "assignment-revision:4";
    assignment.timerIntentId = "timer-intent:1";
    assignment.intentRevision = "2";
    assignment.assignmentEpoch = 3;
    assignment.backendId = "backend:living-room";
    assignment.backendGeneration = 9;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding = {
        "channel:ard-hd",
        "S19.2E-1-1019-10301",
        "canonical-channel-map",
        "mapping-revision:7"};
    assignment.capabilityRevision = "capability-revision:8";
    assignment.backendHealthRevision = "backend-health-revision:12";
    assignment.decisionPolicyVersion = "scheduler-policy:1";
    assignment.decisionEvidence.reasons = {"preferred backend is healthy"};
    assignment.decisionEvidence.warnings = {"failover remains allowed"};
    assignment.decisionEvidence.exclusions = {"backend:maintenance"};
    assignment.decisionEvidence.conflictFacts = {"no overlapping managed timer"};
    assignment.decisionEvidence.decisionScore = 850;
    assignment.createdAt = 1786380000;
    assignment.updatedAt = 1786380060;
    return assignment;
}
}

int main()
{
    auto selected = validSelectedAssignment();
    assert(timerAssignmentValid(selected));
    assert(std::string(timerAssignmentStateName(selected.state)) == "selected");
    assert(std::string(timerAssignmentRoleName(selected.role)) == "primary");
    assert(timerAssignmentActiveOwnershipState(selected.state));
    assert(!timerAssignmentTerminal(selected.state));

    assert(timerAssignmentRevisionMatches(
        "assignment-revision:4",
        selected.assignmentRevision));
    assert(!timerAssignmentRevisionMatches(
        "assignment-revision:3",
        selected.assignmentRevision));

    auto missingIdentity = selected;
    missingIdentity.timerAssignmentId.clear();
    assert(!timerAssignmentValid(missingIdentity));

    auto missingEpoch = selected;
    missingEpoch.assignmentEpoch = 0;
    assert(!timerAssignmentValid(missingEpoch));

    auto missingBackend = selected;
    missingBackend.backendId.clear();
    assert(!timerAssignmentValid(missingBackend));

    auto missingGeneration = selected;
    missingGeneration.backendGeneration = 0;
    assert(!timerAssignmentValid(missingGeneration));

    auto partialChannel = selected;
    partialChannel.channelBinding.mappingRevision.clear();
    assert(!timerAssignmentValid(partialChannel));

    auto unassigned = selected;
    unassigned.state = TimerAssignmentState::unassigned;
    unassigned.backendId.clear();
    unassigned.backendGeneration = 0;
    unassigned.channelBinding = {};
    unassigned.capabilityRevision.clear();
    unassigned.backendHealthRevision.clear();
    unassigned.decisionEvidence.reasons = {"no eligible backend"};
    assert(timerAssignmentValid(unassigned));
    assert(!timerAssignmentActiveOwnershipState(unassigned.state));
    assert(!timerAssignmentTerminal(unassigned.state));

    auto invalidUnassigned = unassigned;
    invalidUnassigned.backendId = "backend:stale";
    assert(!timerAssignmentValid(invalidUnassigned));

    auto bound = selected;
    bound.state = TimerAssignmentState::bound;
    assert(!timerAssignmentValid(bound));
    bound.nativeTimerBindingId = "native-timer-binding:1";
    assert(timerAssignmentValid(bound));

    auto replica = selected;
    replica.role = TimerAssignmentRole::replica;
    assert(timerAssignmentValid(replica));
    assert(std::string(timerAssignmentRoleName(replica.role)) == "replica");

    auto replacement = selected;
    replacement.role = TimerAssignmentRole::replacement;
    assert(timerAssignmentValid(replacement));
    assert(std::string(timerAssignmentRoleName(replacement.role)) == "replacement");

    auto noReason = selected;
    noReason.decisionEvidence.reasons.clear();
    assert(!timerAssignmentValid(noReason));

    auto excessiveEvidence = selected;
    excessiveEvidence.decisionEvidence.warnings.assign(33, "warning");
    assert(!timerAssignmentValid(excessiveEvidence));

    auto badTimestamp = selected;
    badTimestamp.updatedAt = badTimestamp.createdAt - 1;
    assert(!timerAssignmentValid(badTimestamp));

    assert(timerAssignmentCanTransition(
        TimerAssignmentState::proposed,
        TimerAssignmentState::selected));
    assert(timerAssignmentCanTransition(
        TimerAssignmentState::selected,
        TimerAssignmentState::provisioning));
    assert(timerAssignmentCanTransition(
        TimerAssignmentState::provisioning,
        TimerAssignmentState::bound));
    assert(timerAssignmentCanTransition(
        TimerAssignmentState::bound,
        TimerAssignmentState::reconciling));
    assert(timerAssignmentCanTransition(
        TimerAssignmentState::reconciling,
        TimerAssignmentState::superseding));
    assert(timerAssignmentCanTransition(
        TimerAssignmentState::superseding,
        TimerAssignmentState::superseded));
    assert(timerAssignmentCanTransition(
        TimerAssignmentState::unassigned,
        TimerAssignmentState::proposed));
    assert(timerAssignmentCanTransition(
        TimerAssignmentState::cancelRequested,
        TimerAssignmentState::cancelled));
    assert(!timerAssignmentCanTransition(
        TimerAssignmentState::selected,
        TimerAssignmentState::bound));
    assert(!timerAssignmentCanTransition(
        TimerAssignmentState::superseded,
        TimerAssignmentState::selected));

    assert(timerAssignmentActiveOwnershipState(TimerAssignmentState::selected));
    assert(timerAssignmentActiveOwnershipState(TimerAssignmentState::provisioning));
    assert(timerAssignmentActiveOwnershipState(TimerAssignmentState::bound));
    assert(timerAssignmentActiveOwnershipState(TimerAssignmentState::reconciling));
    assert(timerAssignmentActiveOwnershipState(TimerAssignmentState::superseding));
    assert(!timerAssignmentActiveOwnershipState(TimerAssignmentState::proposed));
    assert(!timerAssignmentActiveOwnershipState(TimerAssignmentState::unassigned));

    assert(timerAssignmentTerminal(TimerAssignmentState::superseded));
    assert(timerAssignmentTerminal(TimerAssignmentState::cancelled));
    assert(timerAssignmentTerminal(TimerAssignmentState::failed));
    assert(!timerAssignmentTerminal(TimerAssignmentState::bound));

    return 0;
}
