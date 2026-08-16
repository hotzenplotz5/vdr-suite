#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerModifyOperationPreparationService.h"
#include "NativeTimerModifyReadbackVerificationService.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification(bool enabled = true)
{
    NativeTimerSpecification value;
    value.channelId = "S19.2E-1-1019-10301";
    value.title = "Managed Timer";
    value.directory = "VDR-Suite";
    value.day = "2026-08-18";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1015";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = enabled;
    value.vps = false;
    return value;
}

NativeTimerObservedState observed(bool enabled = true)
{
    const auto spec = specification(enabled);
    NativeTimerObservedState value;
    value.channelId = spec.channelId;
    value.title = spec.title;
    value.directory = spec.directory;
    value.day = spec.day;
    value.weekdays = spec.weekdays;
    value.startTime = spec.startTime;
    value.endTime = spec.endTime;
    value.flags = enabled ? 1 : 0;
    value.priority = spec.priority;
    value.lifetime = spec.lifetime;
    value.enabled = enabled;
    value.vps = spec.vps;
    return value;
}

TimerIntent activeIntent(TimerIntentRepository& repository)
{
    TimerIntent intent;
    intent.timerIntentId = "intent:modify:1";
    intent.state = TimerIntentState::draft;
    intent.createdByActorId = "actor:owner";
    intent.spec.intentType = TimerIntentType::manualWindow;
    intent.spec.ownerActorId = "actor:owner";
    intent.spec.channelRequirement.canonicalChannelId = "channel:ard";
    intent.spec.schedule.startAt = 1000;
    intent.spec.schedule.stopAt = 2000;
    intent.spec.schedule.timezone = "Europe/Berlin";
    intent.createdAt = 100;
    intent.updatedAt = 100;
    intent.expiresAt = 3000;
    const auto created = repository.create(intent);
    assert(created.ok());
    TimerIntent active = created.intent;
    active.state = TimerIntentState::active;
    active.updatedAt = 101;
    const auto updated = repository.update(active, created.intent.intentRevision);
    assert(updated.ok());
    return updated.intent;
}

TimerAssignment boundAssignment(
    TimerAssignmentRepository& repository,
    const TimerIntent& intent)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:modify:1";
    assignment.timerIntentId = intent.timerIntentId;
    assignment.intentRevision = intent.intentRevision;
    assignment.assignmentEpoch = 1;
    assignment.backendId = "backend:1";
    assignment.backendGeneration = 7;
    assignment.state = TimerAssignmentState::bound;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding.canonicalChannelId = "channel:ard";
    assignment.channelBinding.backendChannelId = specification().channelId;
    assignment.channelBinding.mappingSource = "map";
    assignment.channelBinding.mappingRevision = "map:1";
    assignment.capabilityRevision = "cap:1";
    assignment.backendHealthRevision = "health:1";
    assignment.decisionPolicyVersion = "policy:1";
    assignment.decisionEvidence.reasons = {"selected"};
    assignment.nativeTimerBindingId = "binding:modify:1";
    assignment.createdAt = 200;
    assignment.updatedAt = 200;
    const auto created = repository.create(assignment);
    assert(created.ok());
    return created.assignment;
}

NativeTimerBinding bindingFor(
    NativeTimerBindingRepository& repository,
    const TimerAssignment& assignment)
{
    NativeTimerBinding binding;
    binding.nativeTimerBindingId = assignment.nativeTimerBindingId;
    binding.backendId = assignment.backendId;
    binding.backendGeneration = assignment.backendGeneration;
    binding.backendNativeTimerId = "42";
    binding.timerAssignmentId = assignment.timerAssignmentId;
    binding.ownership = NativeTimerBindingOwnership::managed;
    binding.observedState = observed(true);
    binding.observedFingerprint =
        nativeTimerObservedStateFingerprint(binding.observedState);
    binding.lastObservedAt = 250;
    binding.lastVerifiedOperationId = "operation:create:1";
    const auto created = repository.create(binding);
    assert(created.ok());
    return created.binding;
}

NativeTimerModifyOperationPreparationRequest requestFor(
    const TimerIntent& intent,
    const TimerAssignment& assignment,
    const NativeTimerBinding& binding,
    NativeTimerModifyKind kind)
{
    NativeTimerModifyOperationPreparationRequest request;
    request.operationId = kind == NativeTimerModifyKind::update
        ? "operation:update:1" : "operation:toggle:1";
    request.idempotencyKey = kind == NativeTimerModifyKind::update
        ? "idempotency:update:1" : "idempotency:toggle:1";
    request.actorId = "actor:owner";
    request.requestFingerprint = "request:modify:1";
    request.kind = kind;
    request.timerAssignmentId = assignment.timerAssignmentId;
    request.expectedAssignmentRevision = assignment.assignmentRevision;
    request.expectedIntentRevision = intent.intentRevision;
    request.expectedAssignmentEpoch = assignment.assignmentEpoch;
    request.nativeTimerBindingId = binding.nativeTimerBindingId;
    request.expectedBindingRevision = binding.bindingRevision;
    request.expectedBackendId = binding.backendId;
    request.expectedBackendGeneration = binding.backendGeneration;
    request.backendNativeTimerId = binding.backendNativeTimerId;
    request.expectedCurrentFingerprint = binding.observedFingerprint;
    request.expectedSpecification = specification(
        kind == NativeTimerModifyKind::toggle ? false : true);
    if (kind == NativeTimerModifyKind::update)
        request.expectedSpecification.title = "Updated Managed Timer";
    request.requestedAt = 300;
    request.deadline = 900;
    return request;
}

NativeTimerObservation observationFor(
    const NativeTimerBinding& binding,
    const NativeTimerSpecification& expected)
{
    NativeTimerObservation observation;
    observation.backendId = binding.backendId;
    observation.backendGeneration = binding.backendGeneration;
    observation.backendNativeTimerId = binding.backendNativeTimerId;
    observation.observedAt = 350;
    observation.observedState = observed(expected.enabled);
    observation.observedState.title = expected.title;
    observation.observedFingerprint =
        nativeTimerObservedStateFingerprint(observation.observedState);
    return observation;
}
}

int main()
{
    NativeTimerModifyOperationPayload codec;
    codec.kind = NativeTimerModifyKind::toggle;
    codec.timerAssignmentId = "assignment:1";
    codec.expectedAssignmentRevision = "3";
    codec.expectedIntentRevision = "2";
    codec.assignmentEpoch = 1;
    codec.nativeTimerBindingId = "binding:1";
    codec.expectedBindingRevision = "4";
    codec.backendId = "backend:1";
    codec.backendGeneration = 7;
    codec.backendNativeTimerId = "42";
    codec.expectedCurrentFingerprint = "sha256:before";
    codec.expectedSpecification = specification(false);
    const std::string serialized =
        serializeNativeTimerModifyOperationPayload(codec);
    assert(!serialized.empty());
    NativeTimerModifyOperationPayload parsed;
    assert(parseNativeTimerModifyOperationPayload(serialized, parsed));
    assert(parsed.kind == NativeTimerModifyKind::toggle);
    assert(parsed.expectedSpecification.startTime == "0930");
    assert(!parseNativeTimerModifyOperationPayload(
        serialized.substr(0, serialized.size() - 1), parsed));

    Database database;
    assert(database.open(":memory:"));
    TimerIntentRepository intents(database);
    TimerAssignmentRepository assignments(database);
    NativeTimerBindingRepository bindings(database);
    MutationOperationRepository operations(database);
    assert(intents.ensureSchema());
    assert(assignments.ensureSchema());
    assert(bindings.ensureSchema());
    assert(operations.ensureSchema());

    const TimerIntent intent = activeIntent(intents);
    const TimerAssignment assignment = boundAssignment(assignments, intent);
    const NativeTimerBinding binding = bindingFor(bindings, assignment);

    NativeTimerModifyOperationPreparationService preparation(
        intents, assignments, bindings, operations);
    const auto updateRequest = requestFor(
        intent, assignment, binding, NativeTimerModifyKind::update);
    const auto prepared = preparation.prepare(updateRequest);
    assert(prepared.status ==
        NativeTimerModifyOperationPreparationStatus::prepared);
    assert(prepared.operation.actionFamily == "timer.update");
    assert(prepared.operation.expectedRevision == binding.bindingRevision);
    assert(prepared.operation.expectedResourceFingerprint ==
        binding.observedFingerprint);
    assert(preparation.prepare(updateRequest).status ==
        NativeTimerModifyOperationPreparationStatus::alreadyPrepared);

    auto stale = updateRequest;
    stale.expectedCurrentFingerprint = "sha256:stale";
    assert(preparation.prepare(stale).status ==
        NativeTimerModifyOperationPreparationStatus::currentFingerprintConflict);

    auto invalidToggle = requestFor(
        intent, assignment, binding, NativeTimerModifyKind::toggle);
    invalidToggle.expectedSpecification.title = "Changed";
    assert(preparation.prepare(invalidToggle).status ==
        NativeTimerModifyOperationPreparationStatus::toggleShapeConflict);

    NativeTimerModifyReadbackExpectation expectation;
    expectation.operationId = updateRequest.operationId;
    expectation.payload = prepared.payload;
    expectation.readbackNotBefore = 300;
    const auto observation = observationFor(
        binding, updateRequest.expectedSpecification);
    NativeTimerModifyReadbackVerificationService verification(bindings);
    const auto verified = verification.verify(expectation, observation);
    assert(verified.status ==
        NativeTimerModifyReadbackVerificationStatus::verified);
    assert(verified.binding.observedState.title == "Updated Managed Timer");
    assert(verified.binding.lastVerifiedOperationId ==
        updateRequest.operationId);
    assert(verification.verify(expectation, observation).status ==
        NativeTimerModifyReadbackVerificationStatus::alreadyVerified);

    std::cout << "test_native_timer_modify_operation passed\n";
    return 0;
}
