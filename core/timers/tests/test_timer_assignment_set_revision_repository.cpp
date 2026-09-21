#include "Database.h"
#include "TimerAssignmentRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{

TimerAssignment makeSelected(
    const std::string& id,
    const std::string& backendId,
    TimerAssignmentRole role,
    std::int64_t createdAt,
    const std::string& intentRevision = "3")
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = id;
    assignment.timerIntentId = "intent:one";
    assignment.intentRevision = intentRevision;
    assignment.backendId = backendId;
    assignment.backendGeneration = 9;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = role;
    assignment.channelBinding = {
        "channel:ard-hd",
        "S19.2E-1-1019-10301",
        "canonical-channel-map",
        "mapping-revision:7"};
    assignment.capabilityRevision = "capability-revision:8";
    assignment.backendHealthRevision = "backend-health-revision:12";
    assignment.decisionPolicyVersion = "timer-assignment-planner/1";
    assignment.decisionEvidence.reasons = {
        "selected by deterministic planner"};
    assignment.decisionEvidence.conflictFacts = {
        "confirmed_clear"};
    assignment.createdAt = createdAt;
    assignment.updatedAt = createdAt;
    return assignment;
}

void createIntentTable(Database& database)
{
    assert(database.execute(
        "CREATE TABLE timer_intents ("
        "timer_intent_id TEXT PRIMARY KEY NOT NULL,"
        "intent_revision INTEGER NOT NULL CHECK(intent_revision > 0)"
        ");"));
    assert(database.execute(
        "INSERT INTO timer_intents "
        "(timer_intent_id,intent_revision) VALUES ('intent:one',3);"));
}

}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    createIntentTable(database);

    TimerAssignmentRepository repository(database);
    assert(repository.ensureSchema());

    const auto emptySet =
        repository.assignmentSetRevisionForIntent("intent:one");
    assert(emptySet.ok());
    assert(emptySet.assignmentSetRevision == "0");

    assert(repository.assignmentSetRevisionForIntent("").status ==
        TimerAssignmentRepositoryStatus::invalid);
    assert(repository.assignmentSetRevisionForIntent("intent:missing").status ==
        TimerAssignmentRepositoryStatus::intentNotFound);

    TimerAssignment primary = makeSelected(
        "assignment:primary",
        "backend:alpha",
        TimerAssignmentRole::primary,
        100);
    const auto primaryCreated =
        repository.createAgainstAssignmentSetRevision(primary, "0");
    assert(primaryCreated.ok());
    assert(primaryCreated.assignment.assignmentEpoch == 1);

    const auto afterPrimary =
        repository.assignmentSetRevisionForIntent("intent:one");
    assert(afterPrimary.ok());
    assert(afterPrimary.assignmentSetRevision == "1");

    TimerAssignment replica = makeSelected(
        "assignment:replica",
        "backend:beta",
        TimerAssignmentRole::replica,
        110);
    assert(repository.createAgainstAssignmentSetRevision(replica, "0").status ==
        TimerAssignmentRepositoryStatus::conflict);

    const auto afterStaleAttempt =
        repository.assignmentSetRevisionForIntent("intent:one");
    assert(afterStaleAttempt.ok());
    assert(afterStaleAttempt.assignmentSetRevision == "1");

    const auto replicaCreated =
        repository.createAgainstAssignmentSetRevision(replica, "1");
    assert(replicaCreated.ok());
    assert(replicaCreated.assignment.assignmentEpoch == 2);

    const auto afterReplica =
        repository.assignmentSetRevisionForIntent("intent:one");
    assert(afterReplica.ok());
    assert(afterReplica.assignmentSetRevision == "2");

    TimerAssignment ordinary = makeSelected(
        "assignment:ordinary",
        "backend:gamma",
        TimerAssignmentRole::replica,
        120);
    ordinary.state = TimerAssignmentState::proposed;
    const auto ordinaryCreated = repository.create(ordinary);
    assert(ordinaryCreated.ok());
    assert(ordinaryCreated.assignment.assignmentEpoch == 3);

    const auto afterOrdinaryCreate =
        repository.assignmentSetRevisionForIntent("intent:one");
    assert(afterOrdinaryCreate.ok());
    assert(afterOrdinaryCreate.assignmentSetRevision == "3");

    TimerAssignment ordinarySelected = ordinaryCreated.assignment;
    ordinarySelected.state = TimerAssignmentState::selected;
    ordinarySelected.updatedAt += 1;
    const auto ordinaryUpdated = repository.update(
        ordinarySelected,
        ordinaryCreated.assignment.assignmentRevision);
    assert(ordinaryUpdated.ok());

    const auto afterOrdinaryUpdate =
        repository.assignmentSetRevisionForIntent("intent:one");
    assert(afterOrdinaryUpdate.ok());
    assert(afterOrdinaryUpdate.assignmentSetRevision == "4");

    const std::string sharedPlanningFence =
        afterOrdinaryUpdate.assignmentSetRevision;

    TimerAssignment firstConcurrentPlan = makeSelected(
        "assignment:planned-a",
        "backend:delta",
        TimerAssignmentRole::replica,
        130);
    TimerAssignment secondConcurrentPlan = makeSelected(
        "assignment:planned-b",
        "backend:epsilon",
        TimerAssignmentRole::replica,
        140);

    const auto firstPlannedCreate =
        repository.createAgainstAssignmentSetRevision(
            firstConcurrentPlan,
            sharedPlanningFence);
    assert(firstPlannedCreate.ok());

    assert(repository.createAgainstAssignmentSetRevision(
        secondConcurrentPlan,
        sharedPlanningFence).status ==
        TimerAssignmentRepositoryStatus::conflict);

    const auto afterCompetingPlans =
        repository.assignmentSetRevisionForIntent("intent:one");
    assert(afterCompetingPlans.ok());
    assert(afterCompetingPlans.assignmentSetRevision == "5");

    assert(repository.createAgainstAssignmentSetRevision(
        secondConcurrentPlan,
        "not-a-revision").status ==
        TimerAssignmentRepositoryStatus::invalid);

    // Migration/bootstrap proof: assignments that predate first use of the
    // set-revision API seed the fence from the durable maximum assignmentEpoch.
    Database migratedDatabase;
    assert(migratedDatabase.open(":memory:"));
    createIntentTable(migratedDatabase);

    TimerAssignmentRepository migratedRepository(migratedDatabase);
    assert(migratedRepository.ensureSchema());

    TimerAssignment legacy = makeSelected(
        "assignment:legacy",
        "backend:legacy",
        TimerAssignmentRole::primary,
        200);
    const auto legacyCreated = migratedRepository.create(legacy);
    assert(legacyCreated.ok());
    assert(legacyCreated.assignment.assignmentEpoch == 1);

    const auto bootstrapped =
        migratedRepository.assignmentSetRevisionForIntent("intent:one");
    assert(bootstrapped.ok());
    assert(bootstrapped.assignmentSetRevision == "1");

    TimerAssignment legacyProvisioning = legacyCreated.assignment;
    legacyProvisioning.state = TimerAssignmentState::provisioning;
    legacyProvisioning.updatedAt += 1;
    assert(migratedRepository.update(
        legacyProvisioning,
        legacyCreated.assignment.assignmentRevision).ok());

    const auto afterBootstrappedUpdate =
        migratedRepository.assignmentSetRevisionForIntent("intent:one");
    assert(afterBootstrappedUpdate.ok());
    assert(afterBootstrappedUpdate.assignmentSetRevision == "2");

    // Cross-connection proof: the durable revision counter is shared while the
    // fenced expectation itself remains connection-local TEMP state.
    const char* sharedPath =
        "/tmp/vdrsuite_timer_assignment_set_revision_repository_test.sqlite";
    std::remove(sharedPath);

    Database firstConnection;
    Database secondConnection;
    assert(firstConnection.open(sharedPath));
    assert(secondConnection.open(sharedPath));
    createIntentTable(firstConnection);

    TimerAssignmentRepository firstRepository(firstConnection);
    TimerAssignmentRepository secondRepository(secondConnection);
    assert(firstRepository.ensureSchema());
    assert(secondRepository.ensureSchema());

    const auto sharedEmpty =
        firstRepository.assignmentSetRevisionForIntent("intent:one");
    assert(sharedEmpty.ok());
    assert(sharedEmpty.assignmentSetRevision == "0");

    TimerAssignment secondConnectionMutation = makeSelected(
        "assignment:connection-b",
        "backend:connection-b",
        TimerAssignmentRole::replica,
        300);
    secondConnectionMutation.state = TimerAssignmentState::proposed;
    assert(secondRepository.create(secondConnectionMutation).ok());

    const auto afterSecondConnectionMutation =
        firstRepository.assignmentSetRevisionForIntent("intent:one");
    assert(afterSecondConnectionMutation.ok());
    assert(afterSecondConnectionMutation.assignmentSetRevision == "1");

    TimerAssignment staleFirstConnectionPlan = makeSelected(
        "assignment:connection-a-stale",
        "backend:connection-a",
        TimerAssignmentRole::replica,
        310);
    assert(firstRepository.createAgainstAssignmentSetRevision(
        staleFirstConnectionPlan,
        sharedEmpty.assignmentSetRevision).status ==
        TimerAssignmentRepositoryStatus::conflict);

    const std::string sharedFreshFence =
        afterSecondConnectionMutation.assignmentSetRevision;

    TimerAssignment firstConnectionPlan = makeSelected(
        "assignment:connection-a-fresh",
        "backend:connection-a",
        TimerAssignmentRole::replica,
        320);
    TimerAssignment secondConnectionPlan = makeSelected(
        "assignment:connection-b-fresh",
        "backend:connection-c",
        TimerAssignmentRole::replica,
        330);

    assert(firstRepository.createAgainstAssignmentSetRevision(
        firstConnectionPlan,
        sharedFreshFence).ok());
    assert(secondRepository.createAgainstAssignmentSetRevision(
        secondConnectionPlan,
        sharedFreshFence).status ==
        TimerAssignmentRepositoryStatus::conflict);

    const auto afterCrossConnectionRace =
        secondRepository.assignmentSetRevisionForIntent("intent:one");
    assert(afterCrossConnectionRace.ok());
    assert(afterCrossConnectionRace.assignmentSetRevision == "2");

    firstConnection.close();
    secondConnection.close();
    std::remove(sharedPath);

    std::cout
        << "test_timer_assignment_set_revision_repository passed\n";
    return 0;
}
