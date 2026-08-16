#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationCompletionService.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerCreateReadbackEvidence.h"
#include "NativeTimerCreateReadbackVerificationService.h"
#include "NativeTimerSpecification.h"
#include "TimerAssignmentFulfillmentService.h"
#include "TimerAssignmentRepository.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification()
{
    NativeTimerSpecification value;
    value.channelId = "S19.2E-1-1019-10301";
    value.title = "Managed create";
    value.directory = "VDR-Suite";
    value.day = "2026-08-18";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1015";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

TimerAssignment selectedAssignment(
    TimerAssignmentRepository& repository)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:create:outcome";
    assignment.timerIntentId = "intent:create:outcome";
    assignment.intentRevision = "intent-revision:1";
    assignment.assignmentEpoch = 1;
    assignment.backendId = "backend:1";
    assignment.backendGeneration = 7;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding.canonicalChannelId = "channel:ard";
    assignment.channelBinding.backendChannelId = specification().channelId;
    assignment.channelBinding.mappingSource = "map";
    assignment.channelBinding.mappingRevision = "map:1";
    assignment.capabilityRevision = "cap:1";
    assignment.backendHealthRevision = "health:1";
    assignment.decisionPolicyVersion = "policy:1";
    assignment.decisionEvidence.reasons = {"selected"};
    assignment.createdAt = 90;
    assignment.updatedAt = 90;
    const auto created = repository.create(assignment);
    assert(created.ok());
    return created.assignment;
}

NativeTimerObservedState observedState()
{
    const auto expected = specification();
    NativeTimerObservedState value;
    value.channelId = expected.channelId;
    value.title = expected.title;
    value.directory = expected.directory;
    value.day = expected.day;
    value.weekdays = expected.weekdays;
    value.startTime = expected.startTime;
    value.endTime = expected.endTime;
    value.flags = 1;
    value.priority = expected.priority;
    value.lifetime = expected.lifetime;
    value.enabled = true;
    value.vps = false;
    return value;
}
} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));

    MutationOperationRepository operations(database);
    TimerAssignmentRepository assignments(database);
    NativeTimerBindingRepository bindings(database);
    assert(operations.ensureSchema());
    assert(assignments.ensureSchema());
    assert(bindings.ensureSchema());

    const TimerAssignment selected =
        selectedAssignment(assignments);
    TimerAssignmentFulfillmentService fulfillment(
        assignments, bindings);
    const auto started = fulfillment.beginProvisioning(
        selected.timerAssignmentId,
        selected.assignmentRevision,
        selected.intentRevision,
        selected.backendGeneration,
        95);
    assert(started.status ==
        TimerAssignmentFulfillmentStatus::provisioningStarted);
    const TimerAssignment assignment = started.assignment;

    NativeTimerCreateOperationPayload payload;
    payload.timerAssignmentId = assignment.timerAssignmentId;
    payload.expectedAssignmentRevision = assignment.assignmentRevision;
    payload.expectedIntentRevision = assignment.intentRevision;
    payload.assignmentEpoch = assignment.assignmentEpoch;
    payload.nativeTimerBindingId = "binding:create:outcome";
    payload.backendId = assignment.backendId;
    payload.backendGeneration = assignment.backendGeneration;
    payload.expectedSpecification = specification();

    MutationOperation operation;
    operation.operationId = "operation:create:outcome";
    operation.idempotencyKey = "idempotency:create:outcome";
    operation.actorId = "actor:owner";
    operation.backendId = assignment.backendId;
    operation.backendGeneration = assignment.backendGeneration;
    operation.resourceType = "TimerAssignment";
    operation.resourceId = assignment.timerAssignmentId;
    operation.expectedRevision = assignment.assignmentRevision;
    operation.expectedResourceFingerprint =
        nativeTimerSpecificationFingerprint(specification());
    operation.actionFamily = "timer.create";
    operation.requestFingerprint = "request:create:outcome";
    operation.requestedAt = 100;
    operation.deadline = 700;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    operation.state = MutationOperationState::accepted;
    operation.updatedAt = 100;

    MutationOperationPayload durable;
    durable.operationId = operation.operationId;
    durable.payloadType = "native.timer.create";
    durable.payloadVersion = 1;
    durable.payload = serializeNativeTimerCreateOperationPayload(payload);
    durable.payloadFingerprint =
        nativeTimerCreateOperationPayloadFingerprint(payload);
    const auto reserved = operations.reserveWithPayload(operation, durable);
    assert(reserved.ok());

    NativeTimerCreateDispatchClaimRequest claimRequest;
    claimRequest.operationId = operation.operationId;
    claimRequest.expectedOperationRevision =
        reserved.operation.operationRevision;
    claimRequest.timerAssignmentId = assignment.timerAssignmentId;
    claimRequest.nativeTimerBindingId = payload.nativeTimerBindingId;
    claimRequest.backendId = assignment.backendId;
    claimRequest.backendGeneration = assignment.backendGeneration;
    claimRequest.expectedSpecificationFingerprint =
        operation.expectedResourceFingerprint;
    claimRequest.reservation.commandId = "command:create:outcome";
    claimRequest.reservation.requestFingerprint =
        "fingerprint:create:outcome";

    NativeTimerCreateDispatchService dispatch(operations);
    const auto claimed =
        dispatch.claimAfterReservation(claimRequest, 110);
    assert(claimed.status ==
        NativeTimerCreateDispatchClaimStatus::claimed);

    NativeTimerCreateExecutorOutcome outcome;
    outcome.operationId = operation.operationId;
    outcome.operationRevision = claimed.operation.operationRevision;
    outcome.reservation = claimRequest.reservation;
    outcome.category =
        NativeTimerCreateExecutorOutcomeCategory::acceptedUnverified;
    outcome.dispatchStartedAt = 115;
    outcome.completedAt = 120;
    outcome.evidenceReference = "suitebridge:ntcreate:outcome";
    const auto applied = dispatch.applyOutcome(outcome);
    assert(applied.status ==
        NativeTimerCreateDispatchOutcomeStatus::applied);
    assert(applied.expectationPresent);
    assert(applied.expectation.operationState ==
        NativeTimerReadbackOperationState::executedUnverified);
    assert(applied.expectation.readbackNotBefore == 115);
    assert(dispatch.applyOutcome(outcome).status ==
        NativeTimerCreateDispatchOutcomeStatus::alreadyApplied);

    NativeTimerCreateReadbackCandidate candidate;
    candidate.timerAssignmentId = assignment.timerAssignmentId;
    candidate.nativeTimerBindingId = payload.nativeTimerBindingId;
    candidate.observation.backendId = assignment.backendId;
    candidate.observation.backendGeneration =
        assignment.backendGeneration;
    candidate.observation.backendNativeTimerId = "native:42";
    candidate.observation.observedAt = 130;
    candidate.observation.observedState = observedState();
    candidate.observation.observedFingerprint =
        nativeTimerObservedStateFingerprint(
            candidate.observation.observedState);

    NativeTimerCreateReadbackEvidence evidence;
    evidence.backendId = assignment.backendId;
    evidence.backendGeneration = assignment.backendGeneration;
    evidence.observedAt = candidate.observation.observedAt;
    evidence.completeness =
        NativeTimerCreateReadbackCompleteness::complete;
    evidence.candidates = {candidate};

    NativeTimerCreateReadbackVerificationService verification(bindings);
    const auto verified =
        verification.verify(applied.expectation, evidence);
    assert(verified.status ==
        NativeTimerCreateReadbackVerificationStatus::verified);

    const auto bound = fulfillment.bindVerified(
        assignment.timerAssignmentId,
        assignment.assignmentRevision,
        assignment.intentRevision,
        assignment.backendGeneration,
        verified.binding.nativeTimerBindingId,
        verified.binding.bindingRevision,
        140);
    assert(bound.status == TimerAssignmentFulfillmentStatus::bound);
    assert(bound.assignment.state == TimerAssignmentState::bound);

    NativeTimerCreateOperationCompletionService completion(
        operations, assignments, bindings);
    const auto completed =
        completion.complete(applied.expectation, 150);
    assert(completed.status ==
        NativeTimerCreateOperationCompletionStatus::completed);
    assert(completed.operation.state == MutationOperationState::succeeded);
    assert(completion.complete(applied.expectation, 150).status ==
        NativeTimerCreateOperationCompletionStatus::alreadyCompleted);

    std::cout << "test_native_timer_create_outcome_completion passed\n";
    return 0;
}
