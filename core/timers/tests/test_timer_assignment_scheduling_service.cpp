#include "Database.h"
#include "TimerAssignmentSchedulingService.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
TimerIntent makeDraftIntent(const std::string& id)
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
    intent.createdAt = 100;
    intent.updatedAt = 100;
    intent.expiresAt = 3000;
    return intent;
}

TimerIntent createActiveIntent(
    TimerIntentRepository& repository,
    const std::string& id)
{
    const auto created = repository.create(makeDraftIntent(id));
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
    const std::string& backendId = "backend:alpha")
{
    TimerAssignmentPlanningBackendCandidate candidate;
    candidate.backendId = backendId;
    candidate.siteId = "site:home";
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
    candidate.channel.backendChannelId = "S19.2E-1-1019-10301";
    candidate.channel.current = true;
    candidate.channel.ambiguous = false;

    candidate.conflict =
        TimerAssignmentPlanningConflictState::confirmedClear;
    return candidate;
}

TimerAssignmentPrimarySchedulingRequest schedulingRequest(
    const std::string& assignmentId,
    const TimerIntent& intent,
    std::int64_t createdAt)
{
    TimerAssignmentPrimarySchedulingRequest request;
    request.timerAssignmentId = assignmentId;
    request.timerIntentId = intent.timerIntentId;
    request.expectedIntentRevision = intent.intentRevision;
    request.createdAt = createdAt;
    request.candidates = {healthyCandidate()};
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

    const TimerIntent firstIntent =
        createActiveIntent(intentRepository, "intent:first");

    auto firstRequest =
        schedulingRequest("assignment:first", firstIntent, 200);
    const auto first = service.schedulePrimary(firstRequest);
    assert(first.status == TimerAssignmentSchedulingStatus::persisted);
    assert(first.decision.outcome ==
        TimerAssignmentPlanningOutcome::selected);
    assert(first.assignment.state == TimerAssignmentState::selected);
    assert(first.assignment.role == TimerAssignmentRole::primary);
    assert(first.assignment.timerIntentId == firstIntent.timerIntentId);
    assert(first.assignment.intentRevision == firstIntent.intentRevision);
    assert(first.assignment.backendId == "backend:alpha");
    assert(first.assignment.backendGeneration == 7);
    assert(first.assignment.assignmentRevision == "1");
    assert(first.assignment.assignmentEpoch == 1);
    assert(first.assignment.decisionPolicyVersion ==
        timerAssignmentPlanningPolicyVersion());
    assert(first.assignment.nativeTimerBindingId.empty());

    const auto replay = service.schedulePrimary(firstRequest);
    assert(replay.status ==
        TimerAssignmentSchedulingStatus::alreadyPersisted);
    assert(replay.assignment.timerAssignmentId == "assignment:first");
    assert(replay.assignment.assignmentRevision == "1");
    assert(replay.assignment.assignmentEpoch == 1);

    auto conflictingId = firstRequest;
    conflictingId.createdAt += 1;
    const auto idConflict = service.schedulePrimary(conflictingId);
    assert(idConflict.status ==
        TimerAssignmentSchedulingStatus::assignmentIdConflict);

    auto secondPrimary =
        schedulingRequest("assignment:second", firstIntent, 210);
    const auto existingPrimary = service.schedulePrimary(secondPrimary);
    assert(existingPrimary.status ==
        TimerAssignmentSchedulingStatus::activePrimaryExists);
    assert(existingPrimary.assignment.timerAssignmentId ==
        "assignment:first");

    const auto firstList =
        assignmentRepository.listForIntent(firstIntent.timerIntentId);
    assert(firstList.ok());
    assert(firstList.assignments.size() == 1);

    auto staleRevision =
        schedulingRequest("assignment:stale", firstIntent, 220);
    staleRevision.expectedIntentRevision = "1";
    const auto stale = service.schedulePrimary(staleRevision);
    assert(stale.status ==
        TimerAssignmentSchedulingStatus::intentRevisionConflict);
    assert(assignmentRepository.findById("assignment:stale").status ==
        TimerAssignmentRepositoryStatus::notFound);

    const TimerIntent unassignedIntent =
        createActiveIntent(intentRepository, "intent:unassigned");
    auto unassignedRequest =
        schedulingRequest(
            "assignment:unassigned",
            unassignedIntent,
            300);
    unassignedRequest.candidates.front().state =
        TimerAssignmentPlanningBackendState::offline;
    const auto unassigned =
        service.schedulePrimary(unassignedRequest);
    assert(unassigned.status ==
        TimerAssignmentSchedulingStatus::persisted);
    assert(unassigned.decision.outcome ==
        TimerAssignmentPlanningOutcome::unassigned);
    assert(unassigned.assignment.state ==
        TimerAssignmentState::unassigned);
    assert(unassigned.assignment.backendId.empty());
    assert(unassigned.assignment.backendGeneration == 0);
    assert(unassigned.assignment.channelBinding.backendChannelId.empty());
    assert(unassigned.assignment.capabilityRevision.empty());
    assert(unassigned.assignment.backendHealthRevision.empty());
    assert(!unassigned.assignment.decisionEvidence.reasons.empty());
    assert(unassigned.assignment.decisionEvidence.reasons.front() ==
        "no_eligible_backend");

    const TimerIntent invalidPlanningIntent =
        createActiveIntent(intentRepository, "intent:invalid-planning");
    auto invalidPlanningRequest =
        schedulingRequest(
            "assignment:invalid-planning",
            invalidPlanningIntent,
            400);
    invalidPlanningRequest.candidates.push_back(
        invalidPlanningRequest.candidates.front());
    const auto invalidPlanning =
        service.schedulePrimary(invalidPlanningRequest);
    assert(invalidPlanning.status ==
        TimerAssignmentSchedulingStatus::planningInvalid);
    assert(assignmentRepository.findById(
        "assignment:invalid-planning").status ==
        TimerAssignmentRepositoryStatus::notFound);

    const auto draftCreated =
        intentRepository.create(makeDraftIntent("intent:draft"));
    assert(draftCreated.ok());
    auto draftRequest =
        schedulingRequest("assignment:draft", draftCreated.intent, 500);
    const auto draft = service.schedulePrimary(draftRequest);
    assert(draft.status ==
        TimerAssignmentSchedulingStatus::planningInvalid);

    TimerAssignmentPrimarySchedulingRequest missing;
    missing.timerAssignmentId = "assignment:missing";
    missing.timerIntentId = "intent:missing";
    missing.expectedIntentRevision = "1";
    missing.createdAt = 600;
    missing.candidates = {healthyCandidate()};
    const auto notFound = service.schedulePrimary(missing);
    assert(notFound.status ==
        TimerAssignmentSchedulingStatus::intentNotFound);

    TimerAssignmentPrimarySchedulingRequest invalid;
    invalid.timerAssignmentId = "";
    invalid.timerIntentId = firstIntent.timerIntentId;
    invalid.expectedIntentRevision = firstIntent.intentRevision;
    invalid.createdAt = 700;
    const auto invalidResult = service.schedulePrimary(invalid);
    assert(invalidResult.status ==
        TimerAssignmentSchedulingStatus::invalidRequest);

    std::cout
        << "test_timer_assignment_scheduling_service passed\n";
    return 0;
}
