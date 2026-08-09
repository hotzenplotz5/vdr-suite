#include "Database.h"
#include "TimerAssignmentRepository.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{

TimerAssignment makeSelected(
    const std::string& id,
    const std::string& intentRevision = "3")
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = id;
    assignment.timerIntentId = "intent:one";
    assignment.intentRevision = intentRevision;
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
    assignment.decisionEvidence.reasons = {
        "preferred backend is healthy"};
    assignment.decisionEvidence.warnings = {
        "failover remains allowed"};
    assignment.decisionEvidence.exclusions = {
        "backend:maintenance"};
    assignment.decisionEvidence.conflictFacts = {
        "no overlapping managed timer"};
    assignment.decisionEvidence.decisionScore = 850;
    assignment.createdAt = 100;
    assignment.updatedAt = 999;
    return assignment;
}

}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    assert(database.execute(
        "CREATE TABLE timer_intents ("
        "timer_intent_id TEXT PRIMARY KEY NOT NULL,"
        "intent_revision INTEGER NOT NULL CHECK(intent_revision > 0)"
        ");"));
    assert(database.execute(
        "INSERT INTO timer_intents "
        "(timer_intent_id,intent_revision) VALUES ('intent:one',3);"));
    assert(database.execute(
        "INSERT INTO timer_intents "
        "(timer_intent_id,intent_revision) VALUES ('intent:two',1);"));

    TimerAssignmentRepository repository(database);
    assert(repository.ensureSchema());

    TimerAssignment first = makeSelected("assignment:one");
    const auto created = repository.create(first);
    assert(created.ok());
    assert(created.assignment.assignmentRevision == "1");
    assert(created.assignment.assignmentEpoch == 1);
    assert(created.assignment.updatedAt ==
        created.assignment.createdAt);

    const auto loaded = repository.findById("assignment:one");
    assert(loaded.ok());
    assert(loaded.assignment.timerIntentId == "intent:one");
    assert(loaded.assignment.decisionEvidence.warnings.at(0) ==
        "failover remains allowed");

    assert(repository.create(first).status ==
        TimerAssignmentRepositoryStatus::alreadyExists);

    TimerAssignment callerOwnedRevision =
        makeSelected("assignment:caller-revision");
    callerOwnedRevision.assignmentRevision = "99";
    assert(repository.create(callerOwnedRevision).status ==
        TimerAssignmentRepositoryStatus::invalid);

    TimerAssignment callerOwnedEpoch =
        makeSelected("assignment:caller-epoch");
    callerOwnedEpoch.assignmentEpoch = 99;
    assert(repository.create(callerOwnedEpoch).status ==
        TimerAssignmentRepositoryStatus::invalid);

    TimerAssignment staleIntent =
        makeSelected("assignment:stale-intent", "2");
    staleIntent.state = TimerAssignmentState::proposed;
    assert(repository.create(staleIntent).status ==
        TimerAssignmentRepositoryStatus::intentRevisionConflict);

    TimerAssignment missingIntent =
        makeSelected("assignment:missing-intent");
    missingIntent.timerIntentId = "intent:missing";
    assert(repository.create(missingIntent).status ==
        TimerAssignmentRepositoryStatus::intentNotFound);

    TimerAssignment second = makeSelected("assignment:two");
    second.state = TimerAssignmentState::proposed;
    second.createdAt = 110;
    const auto secondCreated = repository.create(second);
    assert(secondCreated.ok());
    assert(secondCreated.assignment.assignmentEpoch == 2);

    const auto list = repository.listForIntent("intent:one");
    assert(list.ok());
    assert(list.assignments.size() == 2);
    assert(list.assignments.at(0).assignmentEpoch == 1);
    assert(list.assignments.at(1).assignmentEpoch == 2);

    const auto active =
        repository.findActivePrimaryForIntent("intent:one");
    assert(active.ok());
    assert(active.assignment.timerAssignmentId == "assignment:one");

    TimerAssignment conflicting = secondCreated.assignment;
    conflicting.state = TimerAssignmentState::selected;
    conflicting.updatedAt += 1;
    assert(repository.update(conflicting, "1").status ==
        TimerAssignmentRepositoryStatus::ownershipConflict);

    TimerAssignment provisioning = created.assignment;
    provisioning.state = TimerAssignmentState::provisioning;
    provisioning.updatedAt += 1;
    const auto provisioned =
        repository.update(provisioning, "1");
    assert(provisioned.ok());
    assert(provisioned.assignment.assignmentRevision == "2");

    TimerAssignment stale = created.assignment;
    stale.state = TimerAssignmentState::provisioning;
    stale.updatedAt += 2;
    const auto conflict = repository.update(stale, "1");
    assert(conflict.status ==
        TimerAssignmentRepositoryStatus::conflict);
    assert(conflict.assignment.assignmentRevision == "2");

    TimerAssignment manufacturedRevision = provisioned.assignment;
    manufacturedRevision.assignmentRevision = "3";
    manufacturedRevision.updatedAt += 1;
    assert(repository.update(manufacturedRevision, "2").status ==
        TimerAssignmentRepositoryStatus::invalid);

    TimerAssignment movedBackend = provisioned.assignment;
    movedBackend.backendId = "backend:bedroom";
    movedBackend.updatedAt += 1;
    assert(repository.update(movedBackend, "2").status ==
        TimerAssignmentRepositoryStatus::invalid);

    TimerAssignment changedRole = provisioned.assignment;
    changedRole.role = TimerAssignmentRole::replica;
    changedRole.updatedAt += 1;
    assert(repository.update(changedRole, "2").status ==
        TimerAssignmentRepositoryStatus::invalid);

    TimerAssignment bound = provisioned.assignment;
    bound.state = TimerAssignmentState::bound;
    bound.updatedAt += 1;
    assert(repository.update(bound, "2").status ==
        TimerAssignmentRepositoryStatus::invalid);
    bound.nativeTimerBindingId = "native-timer-binding:1";
    const auto boundResult = repository.update(bound, "2");
    assert(boundResult.ok());
    assert(boundResult.assignment.assignmentRevision == "3");

    assert(database.execute(
        "UPDATE timer_intents "
        "SET intent_revision=4 WHERE timer_intent_id='intent:one';"));

    TimerAssignment reconcile = boundResult.assignment;
    reconcile.state = TimerAssignmentState::reconciling;
    reconcile.updatedAt += 1;
    assert(repository.update(reconcile, "3").status ==
        TimerAssignmentRepositoryStatus::intentRevisionConflict);

    reconcile.intentRevision = "4";
    const auto reconciled = repository.update(reconcile, "3");
    assert(reconciled.ok());
    assert(reconciled.assignment.assignmentRevision == "4");
    assert(reconciled.assignment.intentRevision == "4");

    TimerAssignment superseding = reconciled.assignment;
    superseding.state = TimerAssignmentState::superseding;
    superseding.updatedAt += 1;
    const auto handover = repository.update(superseding, "4");
    assert(handover.ok());

    TimerAssignment superseded = handover.assignment;
    superseded.state = TimerAssignmentState::superseded;
    superseded.updatedAt += 1;
    const auto terminal = repository.update(superseded, "5");
    assert(terminal.ok());
    assert(terminal.assignment.assignmentRevision == "6");

    TimerAssignment mutateTerminal = terminal.assignment;
    mutateTerminal.decisionEvidence.reasons = {"try mutation"};
    mutateTerminal.updatedAt += 1;
    assert(repository.update(mutateTerminal, "6").status ==
        TimerAssignmentRepositoryStatus::invalid);

    conflicting.intentRevision = "4";
    conflicting.updatedAt += 1;
    const auto promoted = repository.update(conflicting, "1");
    assert(promoted.ok());
    assert(promoted.assignment.assignmentRevision == "2");

    TimerAssignment replica = makeSelected("assignment:replica", "4");
    replica.role = TimerAssignmentRole::replica;
    replica.createdAt = 120;
    const auto replicaCreated = repository.create(replica);
    assert(replicaCreated.ok());
    assert(replicaCreated.assignment.assignmentEpoch == 3);

    assert(repository.findById("missing").status ==
        TimerAssignmentRepositoryStatus::notFound);
    assert(repository.findActivePrimaryForIntent("intent:two").status ==
        TimerAssignmentRepositoryStatus::notFound);

    std::cout << "test_timer_assignment_repository passed\n";
    return 0;
}
