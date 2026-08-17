#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "TimerAssignmentReassignmentService.h"
#include "TimerAssignmentSchedulingService.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

namespace
{
TimerIntent activeIntent(TimerIntentRepository& repository, const std::string& id)
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
    intent.spec.assignmentPolicy.allowFailover = true;
    intent.createdAt = 100;
    intent.updatedAt = 100;
    intent.expiresAt = 3000;
    const auto created = repository.create(intent);
    assert(created.ok());
    TimerIntent next = created.intent;
    next.state = TimerIntentState::active;
    next.updatedAt = 101;
    const auto updated = repository.update(next, created.intent.intentRevision);
    assert(updated.ok());
    return updated.intent;
}

TimerAssignmentPlanningBackendCandidate candidate(
    const std::string& backend,
    std::uint64_t generation)
{
    TimerAssignmentPlanningBackendCandidate value;
    value.backendId = backend;
    value.siteId = "site:" + backend;
    value.currentBackendGeneration = generation;
    value.state = TimerAssignmentPlanningBackendState::online;
    value.writeAllowed = true;
    value.executionAuthorityCurrent = true;
    value.executionAuthorityFence = "authority:" + std::to_string(generation);
    value.capability.backendGeneration = generation;
    value.capability.revision = "capability:" + std::to_string(generation);
    value.capability.current = true;
    value.capability.timerCreate = true;
    value.capability.timerReadback = true;
    value.health.backendGeneration = generation;
    value.health.revision = "health:" + std::to_string(generation);
    value.health.current = true;
    value.health.state = TimerAssignmentPlanningHealthState::healthy;
    value.health.timerWritesAvailable = true;
    value.channel.backendGeneration = generation;
    value.channel.mappingRevision = "mapping:" + std::to_string(generation);
    value.channel.mappingSource = "canonical-channel-map";
    value.channel.canonicalChannelId = "channel:ard";
    value.channel.backendChannelId = "native:" + backend;
    value.channel.current = true;
    value.conflict = TimerAssignmentPlanningConflictState::confirmedClear;
    return value;
}

TimerAssignment primary(
    TimerAssignmentSchedulingService& scheduling,
    const TimerIntent& intent,
    const std::string& id,
    std::int64_t createdAt)
{
    TimerAssignmentPrimarySchedulingRequest request;
    request.timerAssignmentId = id;
    request.timerIntentId = intent.timerIntentId;
    request.expectedIntentRevision = intent.intentRevision;
    request.createdAt = createdAt;
    request.candidates = {candidate("backend:alpha", 7)};
    const auto scheduled = scheduling.schedulePrimary(request);
    assert(scheduled.status == TimerAssignmentSchedulingStatus::persisted);
    return scheduled.assignment;
}

TimerAssignmentReassignmentRequest requestFor(
    const TimerAssignment& old,
    const std::string& replacementId,
    std::int64_t createdAt)
{
    TimerAssignmentReassignmentRequest request;
    request.replacementTimerAssignmentId = replacementId;
    request.oldTimerAssignmentId = old.timerAssignmentId;
    request.expectedOldAssignmentRevision = old.assignmentRevision;
    request.expectedOldAssignmentEpoch = old.assignmentEpoch;
    request.expectedIntentRevision = old.intentRevision;
    request.expectedOldBackendId = old.backendId;
    request.expectedOldBackendGeneration = old.backendGeneration;
    request.oldNativeOutcome =
        TimerAssignmentReassignmentNativeOutcome::beforeDispatch;
    request.reason = "backend unavailable before native dispatch";
    request.createdAt = createdAt;
    request.candidates = {
        candidate("backend:alpha", 7),
        candidate("backend:beta", 11)};
    return request;
}

struct VerifiedAbsentFixture
{
    TimerAssignment assignment;
    NativeTimerBinding binding;
    vdrsuite::operations::MutationOperation operation;
};

VerifiedAbsentFixture verifiedAbsentFixture(
    TimerAssignmentRepository& assignments,
    NativeTimerBindingRepository& bindings,
    vdrsuite::operations::MutationOperationRepository& operations,
    const TimerAssignment& selected,
    const std::string& bindingId,
    const std::string& operationId,
    bool outcomeUnknown)
{
    TimerAssignment provisioning = selected;
    provisioning.state = TimerAssignmentState::provisioning;
    provisioning.updatedAt += 1;
    const auto provisioned = assignments.update(
        provisioning, selected.assignmentRevision);
    assert(provisioned.ok());

    NativeTimerBinding binding;
    binding.nativeTimerBindingId = bindingId;
    binding.backendId = selected.backendId;
    binding.backendGeneration = selected.backendGeneration;
    binding.backendNativeTimerId = "native:" + bindingId;
    binding.timerAssignmentId = selected.timerAssignmentId;
    binding.ownership = NativeTimerBindingOwnership::managed;
    binding.observedState.channelId = selected.channelBinding.backendChannelId;
    binding.observedState.title = "controlled test timer";
    binding.observedState.day = "2026-08-18";
    binding.observedState.weekdays = "-------";
    binding.observedState.startTime = "1200";
    binding.observedState.endTime = "1300";
    binding.observedState.recording = false;
    binding.observedFingerprint = nativeTimerObservedStateFingerprint(
        binding.observedState);
    binding.lastObservedAt = provisioned.assignment.updatedAt + 10;
    binding.lastVerifiedOperationId = operationId;
    binding.missingSince = binding.lastObservedAt;
    binding.driftState = NativeTimerBindingDriftState::expectedTransition;
    const auto boundBinding = bindings.create(binding);
    assert(boundBinding.ok());

    TimerAssignment bound = provisioned.assignment;
    bound.state = TimerAssignmentState::bound;
    bound.nativeTimerBindingId = boundBinding.binding.nativeTimerBindingId;
    bound.updatedAt = boundBinding.binding.lastObservedAt + 1;
    const auto boundAssignment = assignments.update(
        bound, provisioned.assignment.assignmentRevision);
    assert(boundAssignment.ok());

    using namespace vdrsuite::operations;
    MutationOperation operation;
    operation.operationId = operationId;
    operation.idempotencyKey = "idem:" + operationId;
    operation.actorId = "actor:reconcile";
    operation.backendId = selected.backendId;
    operation.backendGeneration = selected.backendGeneration;
    operation.resourceType = "NativeTimerBinding";
    operation.resourceId = bindingId;
    operation.expectedRevision = boundBinding.binding.bindingRevision;
    operation.expectedResourceFingerprint = boundBinding.binding.observedFingerprint;
    operation.actionFamily = "timer.delete";
    operation.requestFingerprint = "request:" + operationId;
    operation.requestedAt = boundAssignment.assignment.updatedAt + 1;
    operation.deadline = operation.requestedAt + 100;
    operation.verificationPolicy = MutationOperationVerificationPolicy::readbackRequired;
    operation.updatedAt = operation.requestedAt;
    auto durable = operations.reserve(operation);
    assert(durable.ok());
    durable = operations.transition(
        operationId, durable.operation.operationRevision,
        MutationOperationState::accepted, MutationOperationState::dispatching,
        "dispatch", operation.requestedAt + 1);
    assert(durable.ok());
    if (outcomeUnknown)
    {
        durable = operations.transition(
            operationId, durable.operation.operationRevision,
            MutationOperationState::dispatching,
            MutationOperationState::outcomeUnknown,
            "transport-timeout", operation.requestedAt + 2);
    }
    else
    {
        durable = operations.transition(
            operationId, durable.operation.operationRevision,
            MutationOperationState::dispatching,
            MutationOperationState::executedUnverified,
            "accepted", operation.requestedAt + 2);
        assert(durable.ok());
        durable = operations.transition(
            operationId, durable.operation.operationRevision,
            MutationOperationState::executedUnverified,
            MutationOperationState::succeeded,
            "verified-absence", operation.requestedAt + 3);
    }
    assert(durable.ok());

    return {
        boundAssignment.assignment,
        boundBinding.binding,
        durable.operation};
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    TimerIntentRepository intents(database);
    TimerAssignmentRepository assignments(database);
    NativeTimerBindingRepository bindings(database);
    vdrsuite::operations::MutationOperationRepository operations(database);
    assert(intents.ensureSchema());
    assert(assignments.ensureSchema());
    assert(bindings.ensureSchema());
    assert(operations.ensureSchema());

    TimerAssignmentSchedulingService scheduling(intents, assignments);
    TimerAssignmentReassignmentService service(
        intents, assignments, bindings, operations);

    const TimerIntent intent = activeIntent(intents, "intent:handover");
    const TimerAssignment old = primary(
        scheduling, intent, "assignment:old", 200);
    const auto request = requestFor(old, "assignment:replacement", 300);

    const auto replaced = service.reassign(request);
    assert(replaced.status == TimerAssignmentReassignmentStatus::persisted);
    assert(replaced.oldAssignment.state == TimerAssignmentState::superseded);
    assert(replaced.oldAssignment.assignmentRevision == "2");
    assert(replaced.replacementAssignment.role
        == TimerAssignmentRole::replacement);
    assert(replaced.replacementAssignment.state == TimerAssignmentState::selected);
    assert(replaced.replacementAssignment.backendId == "backend:beta");
    assert(replaced.replacementAssignment.assignmentEpoch == 2);
    assert(replaced.evidence.oldAssignmentEpoch == 1);
    assert(replaced.evidence.newAssignmentEpoch == 2);
    assert(replaced.evidence.oldNativeOutcome
        == TimerAssignmentReassignmentNativeOutcome::beforeDispatch);

    const auto active = assignments.findActivePrimaryForIntent(intent.timerIntentId);
    assert(active.ok());
    assert(active.assignment.timerAssignmentId == "assignment:replacement");
    TimerAssignmentPrimarySchedulingRequest duplicatePrimary;
    duplicatePrimary.timerAssignmentId = "assignment:duplicate-primary";
    duplicatePrimary.timerIntentId = intent.timerIntentId;
    duplicatePrimary.expectedIntentRevision = intent.intentRevision;
    duplicatePrimary.createdAt = 350;
    duplicatePrimary.candidates = {candidate("backend:gamma", 12)};
    assert(scheduling.schedulePrimary(duplicatePrimary).status
        == TimerAssignmentSchedulingStatus::activePrimaryExists);
    const auto all = assignments.listForIntent(intent.timerIntentId);
    assert(all.ok());
    assert(all.assignments.size() == 2);

    const auto replay = service.reassign(request);
    assert(replay.status == TimerAssignmentReassignmentStatus::alreadyPersisted);
    assert(replay.replacementAssignment.assignmentEpoch == 2);
    auto incompatibleReplay = request;
    incompatibleReplay.reason = "different reason";
    assert(service.reassign(incompatibleReplay).status
        == TimerAssignmentReassignmentStatus::replacementIdConflict);

    const TimerIntent staleIntent = activeIntent(intents, "intent:stale");
    const TimerAssignment staleOld = primary(
        scheduling, staleIntent, "assignment:stale-old", 400);
    auto stale = requestFor(staleOld, "assignment:stale-replacement", 500);
    stale.expectedOldAssignmentEpoch += 1;
    assert(service.reassign(stale).status
        == TimerAssignmentReassignmentStatus::assignmentEpochConflict);
    const auto stillOld = assignments.findById(staleOld.timerAssignmentId);
    assert(stillOld.ok());
    assert(stillOld.assignment.state == TimerAssignmentState::selected);
    assert(assignments.findById(stale.replacementTimerAssignmentId).status
        == TimerAssignmentRepositoryStatus::notFound);

    TimerAssignment genericReplacement = replaced.replacementAssignment;
    genericReplacement.timerAssignmentId = "assignment:generic-replacement";
    genericReplacement.assignmentRevision.clear();
    genericReplacement.assignmentEpoch = 0;
    genericReplacement.createdAt = 600;
    genericReplacement.updatedAt = 600;
    assert(assignments.create(genericReplacement).status
        == TimerAssignmentRepositoryStatus::ownershipConflict);

    const TimerIntent noTargetIntent = activeIntent(intents, "intent:no-target");
    const TimerAssignment noTargetOld = primary(
        scheduling, noTargetIntent, "assignment:no-target-old", 700);
    auto noTarget = requestFor(
        noTargetOld, "assignment:no-target-replacement", 800);
    noTarget.candidates = {candidate("backend:alpha", 7)};
    assert(service.reassign(noTarget).status
        == TimerAssignmentReassignmentStatus::noEligibleBackend);
    assert(assignments.findById(noTargetOld.timerAssignmentId).assignment.state
        == TimerAssignmentState::selected);

    const TimerIntent absentIntent = activeIntent(intents, "intent:absent");
    const TimerAssignment absentSelected = primary(
        scheduling, absentIntent, "assignment:absent-old", 900);
    const auto absent = verifiedAbsentFixture(
        assignments,
        bindings,
        operations,
        absentSelected,
        "binding:absent",
        "operation:delete-absent",
        false);
    auto absentRequest = requestFor(
        absent.assignment, "assignment:absent-replacement", 1000);
    absentRequest.oldNativeOutcome =
        TimerAssignmentReassignmentNativeOutcome::verifiedAbsent;
    absentRequest.oldOperationId = absent.operation.operationId;
    absentRequest.expectedOldOperationRevision =
        absent.operation.operationRevision;
    absentRequest.oldNativeTimerBindingId = absent.binding.nativeTimerBindingId;
    absentRequest.expectedOldBindingRevision = absent.binding.bindingRevision;
    const auto absentReplacement = service.reassign(absentRequest);
    assert(absentReplacement.status
        == TimerAssignmentReassignmentStatus::persisted);
    assert(absentReplacement.evidence.oldNativeOutcome
        == TimerAssignmentReassignmentNativeOutcome::verifiedAbsent);
    assert(absentReplacement.evidence.oldOperationId
        == "operation:delete-absent");

    const TimerIntent unknownIntent = activeIntent(intents, "intent:unknown");
    const TimerAssignment unknownSelected = primary(
        scheduling, unknownIntent, "assignment:unknown-old", 1100);
    const auto unknown = verifiedAbsentFixture(
        assignments,
        bindings,
        operations,
        unknownSelected,
        "binding:unknown",
        "operation:delete-unknown",
        true);
    auto unknownRequest = requestFor(
        unknown.assignment, "assignment:unknown-replacement", 1200);
    unknownRequest.oldNativeOutcome =
        TimerAssignmentReassignmentNativeOutcome::verifiedAbsent;
    unknownRequest.oldOperationId = unknown.operation.operationId;
    unknownRequest.expectedOldOperationRevision =
        unknown.operation.operationRevision;
    unknownRequest.oldNativeTimerBindingId = unknown.binding.nativeTimerBindingId;
    unknownRequest.expectedOldBindingRevision = unknown.binding.bindingRevision;
    assert(service.reassign(unknownRequest).status
        == TimerAssignmentReassignmentStatus::operationConflict);
    assert(assignments.findById(unknown.assignment.timerAssignmentId).assignment.state
        == TimerAssignmentState::bound);
    assert(assignments.findById(unknownRequest.replacementTimerAssignmentId).status
        == TimerAssignmentRepositoryStatus::notFound);

    std::cout << "test_timer_assignment_reassignment_service passed\n";
    return 0;
}
