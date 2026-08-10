#include "Database.h"
#include "TimerAssignmentSchedulingService.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
TimerIntent makeDraftIntent(
    const std::string& id,
    std::uint32_t desiredAssignments = 2,
    bool simultaneousRecordingIntentional = true)
{
    TimerIntent intent;
    intent.timerIntentId = id;
    intent.state = TimerIntentState::draft;
    intent.createdByActorId = "actor:create";
    intent.spec.intentType = TimerIntentType::manualWindow;
    intent.spec.ownerActorId = "actor:owner";
    intent.spec.channelRequirement.canonicalChannelId = "channel:ard";
    intent.spec.schedule.startAt = 1000;
    intent.spec.schedule.stopAt = 2000;
    intent.spec.schedule.timezone = "Europe/Berlin";
    intent.spec.assignmentPolicy.preferredBackendIds = {
        "backend:alpha",
        "backend:beta"};
    intent.spec.replicaPolicy.desiredAssignments = desiredAssignments;
    intent.spec.replicaPolicy.requireBackendDiversity = true;
    intent.spec.replicaPolicy.requireSiteDiversity = true;
    intent.spec.replicaPolicy.simultaneousRecordingIntentional =
        simultaneousRecordingIntentional;
    intent.spec.replicaPolicy.rationale = "explicit two-site redundancy";
    intent.createdAt = 100;
    intent.updatedAt = 100;
    intent.expiresAt = 3000;
    return intent;
}

TimerIntent createActiveIntent(
    TimerIntentRepository& repository,
    const std::string& id,
    std::uint32_t desiredAssignments = 2,
    bool simultaneousRecordingIntentional = true)
{
    const auto created = repository.create(
        makeDraftIntent(
            id,
            desiredAssignments,
            simultaneousRecordingIntentional));
    assert(created.ok());

    TimerIntent active = created.intent;
    active.state = TimerIntentState::active;
    active.updatedAt += 1;
    const auto activated =
        repository.update(active, created.intent.intentRevision);
    assert(activated.ok());
    return activated.intent;
}

TimerAssignmentPlanningBackendCandidate healthyCandidate(
    const std::string& backendId,
    const std::string& siteId)
{
    TimerAssignmentPlanningBackendCandidate candidate;
    candidate.backendId = backendId;
    candidate.siteId = siteId;
    candidate.currentBackendGeneration = 7;
    candidate.state = TimerAssignmentPlanningBackendState::online;
    candidate.writeAllowed = true;
    candidate.executionAuthorityCurrent = true;
    candidate.executionAuthorityFence = "authority:fence:7";

    candidate.capability.backendGeneration = 7;
    candidate.capability.revision = "capability:7";
    candidate.capability.current = true;
    candidate.capability.timerCreate = true;
    candidate.capability.timerReadback = true;

    candidate.health.backendGeneration = 7;
    candidate.health.revision = "health:7";
    candidate.health.current = true;
    candidate.health.state =
        TimerAssignmentPlanningHealthState::healthy;
    candidate.health.timerWritesAvailable = true;

    candidate.channel.backendGeneration = 7;
    candidate.channel.mappingRevision = "mapping:7";
    candidate.channel.mappingSource = "canonical-channel-map";
    candidate.channel.canonicalChannelId = "channel:ard";
    candidate.channel.backendChannelId =
        backendId == "backend:alpha"
            ? "S19.2E-1-1019-10301"
            : "S19.2E-1-1019-10302";
    candidate.channel.current = true;
    candidate.channel.ambiguous = false;

    candidate.conflict =
        TimerAssignmentPlanningConflictState::confirmedClear;
    return candidate;
}

TimerAssignmentPrimarySchedulingRequest primaryRequest(
    const std::string& assignmentId,
    const TimerIntent& intent,
    std::int64_t createdAt)
{
    TimerAssignmentPrimarySchedulingRequest request;
    request.timerAssignmentId = assignmentId;
    request.timerIntentId = intent.timerIntentId;
    request.expectedIntentRevision = intent.intentRevision;
    request.createdAt = createdAt;
    request.candidates = {
        healthyCandidate("backend:alpha", "site:home")};
    return request;
}

TimerAssignmentReplicaSchedulingRequest replicaRequest(
    const std::string& assignmentId,
    const TimerIntent& intent,
    std::int64_t createdAt)
{
    TimerAssignmentReplicaSchedulingRequest request;
    request.timerAssignmentId = assignmentId;
    request.timerIntentId = intent.timerIntentId;
    request.expectedIntentRevision = intent.intentRevision;
    request.createdAt = createdAt;
    request.candidates = {
        healthyCandidate("backend:alpha", "site:home"),
        healthyCandidate("backend:beta", "site:remote")};
    return request;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    TimerIntentRepository intentRepository(database);
    TimerAssignmentRepository assignmentRepository(database);
    assert(intentRepository.ensureSchema());
    assert(assignmentRepository.ensureSchema());

    TimerAssignmentSchedulingService service(
        intentRepository,
        assignmentRepository);

    const TimerIntent replicatedIntent =
        createActiveIntent(intentRepository, "intent:replicated");

    const auto primary = service.schedulePrimary(
        primaryRequest("assignment:primary", replicatedIntent, 200));
    assert(primary.status == TimerAssignmentSchedulingStatus::persisted);
    assert(primary.assignment.role == TimerAssignmentRole::primary);
    assert(primary.assignment.backendId == "backend:alpha");
    assert(primary.assignment.assignmentEpoch == 1);

    const auto beforeReplicaSet =
        assignmentRepository.assignmentSetRevisionForIntent(
            replicatedIntent.timerIntentId);
    assert(beforeReplicaSet.ok());
    assert(beforeReplicaSet.assignmentSetRevision == "1");

    auto firstReplicaRequest =
        replicaRequest("assignment:replica", replicatedIntent, 210);
    const auto replica = service.scheduleReplica(firstReplicaRequest);
    assert(replica.status == TimerAssignmentSchedulingStatus::persisted);
    assert(replica.decision.outcome ==
        TimerAssignmentPlanningOutcome::selected);
    assert(replica.assignment.state == TimerAssignmentState::selected);
    assert(replica.assignment.role == TimerAssignmentRole::replica);
    assert(replica.assignment.backendId == "backend:beta");
    assert(replica.assignment.assignmentEpoch == 2);
    assert(replica.assignment.assignmentRevision == "1");

    const auto afterReplicaSet =
        assignmentRepository.assignmentSetRevisionForIntent(
            replicatedIntent.timerIntentId);
    assert(afterReplicaSet.ok());
    assert(afterReplicaSet.assignmentSetRevision == "2");

    const auto replay = service.scheduleReplica(firstReplicaRequest);
    assert(replay.status ==
        TimerAssignmentSchedulingStatus::alreadyPersisted);
    assert(replay.assignment.timerAssignmentId == "assignment:replica");
    assert(replay.assignment.assignmentEpoch == 2);

    auto conflictingId = firstReplicaRequest;
    conflictingId.createdAt += 1;
    const auto idConflict = service.scheduleReplica(conflictingId);
    assert(idConflict.status ==
        TimerAssignmentSchedulingStatus::assignmentIdConflict);

    const auto targetSatisfied = service.scheduleReplica(
        replicaRequest(
            "assignment:replica-extra",
            replicatedIntent,
            220));
    assert(targetSatisfied.status ==
        TimerAssignmentSchedulingStatus::replicaTargetSatisfied);
    assert(targetSatisfied.ok());
    assert(targetSatisfied.decision.outcome ==
        TimerAssignmentPlanningOutcome::unassigned);
    assert(!targetSatisfied.decision.decisionEvidence.reasons.empty());
    assert(targetSatisfied.decision.decisionEvidence.reasons.front() ==
        "replica_target_satisfied");
    assert(targetSatisfied.assignment.timerAssignmentId.empty());

    const auto afterSatisfiedList =
        assignmentRepository.listForIntent(replicatedIntent.timerIntentId);
    assert(afterSatisfiedList.ok());
    assert(afterSatisfiedList.assignments.size() == 2);
    const auto afterSatisfiedSet =
        assignmentRepository.assignmentSetRevisionForIntent(
            replicatedIntent.timerIntentId);
    assert(afterSatisfiedSet.ok());
    assert(afterSatisfiedSet.assignmentSetRevision == "2");
    assert(assignmentRepository.findById(
        "assignment:replica-extra").status ==
        TimerAssignmentRepositoryStatus::notFound);

    const TimerIntent unassignedIntent =
        createActiveIntent(intentRepository, "intent:unassigned-replica");
    const auto unassignedPrimary = service.schedulePrimary(
        primaryRequest(
            "assignment:unassigned-primary",
            unassignedIntent,
            300));
    assert(unassignedPrimary.status ==
        TimerAssignmentSchedulingStatus::persisted);

    auto unassignedReplicaRequest =
        replicaRequest(
            "assignment:unassigned-replica",
            unassignedIntent,
            310);
    unassignedReplicaRequest.candidates.at(1).state =
        TimerAssignmentPlanningBackendState::offline;
    const auto unassignedReplica =
        service.scheduleReplica(unassignedReplicaRequest);
    assert(unassignedReplica.status ==
        TimerAssignmentSchedulingStatus::persisted);
    assert(unassignedReplica.decision.outcome ==
        TimerAssignmentPlanningOutcome::unassigned);
    assert(unassignedReplica.assignment.state ==
        TimerAssignmentState::unassigned);
    assert(unassignedReplica.assignment.role ==
        TimerAssignmentRole::replica);
    assert(unassignedReplica.assignment.backendId.empty());
    assert(unassignedReplica.assignment.assignmentEpoch == 2);
    assert(!unassignedReplica.assignment.decisionEvidence.reasons.empty());
    assert(unassignedReplica.assignment.decisionEvidence.reasons.front() ==
        "no_eligible_backend");

    const auto unassignedReplay =
        service.scheduleReplica(unassignedReplicaRequest);
    assert(unassignedReplay.status ==
        TimerAssignmentSchedulingStatus::alreadyPersisted);
    assert(unassignedReplay.assignment.assignmentEpoch == 2);

    auto staleRevisionRequest =
        replicaRequest(
            "assignment:stale-replica",
            replicatedIntent,
            400);
    staleRevisionRequest.expectedIntentRevision = "1";
    const auto staleRevision = service.scheduleReplica(staleRevisionRequest);
    assert(staleRevision.status ==
        TimerAssignmentSchedulingStatus::intentRevisionConflict);
    assert(assignmentRepository.findById(
        "assignment:stale-replica").status ==
        TimerAssignmentRepositoryStatus::notFound);

    const TimerIntent invalidReplicaPolicyIntent =
        createActiveIntent(
            intentRepository,
            "intent:invalid-replica-policy",
            1,
            false);
    const auto invalidReplica = service.scheduleReplica(
        replicaRequest(
            "assignment:invalid-replica",
            invalidReplicaPolicyIntent,
            500));
    assert(invalidReplica.status ==
        TimerAssignmentSchedulingStatus::planningInvalid);
    assert(assignmentRepository.findById(
        "assignment:invalid-replica").status ==
        TimerAssignmentRepositoryStatus::notFound);

    TimerAssignmentReplicaSchedulingRequest missing;
    missing.timerAssignmentId = "assignment:missing-replica";
    missing.timerIntentId = "intent:missing";
    missing.expectedIntentRevision = "1";
    missing.createdAt = 600;
    missing.candidates = {
        healthyCandidate("backend:alpha", "site:home"),
        healthyCandidate("backend:beta", "site:remote")};
    const auto notFound = service.scheduleReplica(missing);
    assert(notFound.status ==
        TimerAssignmentSchedulingStatus::intentNotFound);

    TimerAssignmentReplicaSchedulingRequest invalid;
    invalid.timerAssignmentId.clear();
    invalid.timerIntentId = replicatedIntent.timerIntentId;
    invalid.expectedIntentRevision = replicatedIntent.intentRevision;
    invalid.createdAt = 700;
    const auto invalidResult = service.scheduleReplica(invalid);
    assert(invalidResult.status ==
        TimerAssignmentSchedulingStatus::invalidRequest);

    assert(std::string(timerAssignmentSchedulingStatusName(
        TimerAssignmentSchedulingStatus::replicaTargetSatisfied)) ==
        "replica_target_satisfied");
    assert(std::string(timerAssignmentSchedulingStatusName(
        TimerAssignmentSchedulingStatus::assignmentSetConflict)) ==
        "assignment_set_conflict");

    std::cout
        << "test_timer_assignment_replica_scheduling_service passed\n";
    return 0;
}
