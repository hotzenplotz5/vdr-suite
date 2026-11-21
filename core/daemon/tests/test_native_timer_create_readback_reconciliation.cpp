#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationCompletionService.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerCreateReadbackEvidence.h"
#include "NativeTimerCreateReadbackReconciliation.h"
#include "NativeTimerCreateReadbackVerificationService.h"
#include "NativeTimerSpecification.h"
#include "TimerAssignmentFulfillmentService.h"
#include "TimerAssignmentRepository.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::daemon;
using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification()
{
    NativeTimerSpecification value;
    value.channelId = "S19.2E-1-1019-10301";
    value.title = "Phase 69 readback reconciliation";
    value.directory = "VDR-Suite";
    value.day = "2026-09-25";
    value.weekdays = "-------";
    value.startTime = "1530";
    value.endTime = "1615";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

TimerAssignment selectedAssignment(TimerAssignmentRepository& repository)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:phase69:reconciliation";
    assignment.timerIntentId = "intent:phase69:reconciliation";
    assignment.intentRevision = "1";
    assignment.backendId = "default";
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
    value.enabled = expected.enabled;
    value.vps = expected.vps;
    return value;
}

NativeTimerCreateReadbackEvidence readbackEvidence(
    const TimerAssignment& assignment,
    const std::string& nativeTimerBindingId,
    std::int64_t observedAt)
{
    NativeTimerCreateReadbackCandidate candidate;
    candidate.timerAssignmentId = assignment.timerAssignmentId;
    candidate.nativeTimerBindingId = nativeTimerBindingId;
    candidate.observation.backendId = assignment.backendId;
    candidate.observation.backendGeneration = assignment.backendGeneration;
    candidate.observation.backendNativeTimerId = "native:phase69:42";
    candidate.observation.observedAt = observedAt;
    candidate.observation.observedState = observedState();
    candidate.observation.observedFingerprint =
        nativeTimerObservedStateFingerprint(candidate.observation.observedState);

    NativeTimerCreateReadbackEvidence evidence;
    evidence.backendId = assignment.backendId;
    evidence.backendGeneration = assignment.backendGeneration;
    evidence.observedAt = observedAt;
    evidence.completeness = NativeTimerCreateReadbackCompleteness::complete;
    evidence.candidates = {candidate};
    return evidence;
}
} // namespace

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
        "(timer_intent_id,intent_revision) "
        "VALUES ('intent:phase69:reconciliation',1);"));

    MutationOperationRepository operations(database);
    TimerAssignmentRepository assignments(database);
    NativeTimerBindingRepository bindings(database);
    assert(operations.ensureSchema());
    assert(assignments.ensureSchema());
    assert(bindings.ensureSchema());

    const TimerAssignment selected = selectedAssignment(assignments);
    TimerAssignmentFulfillmentService fulfillment(assignments, bindings);
    const auto provisioning = fulfillment.beginProvisioning(
        selected.timerAssignmentId,
        selected.assignmentRevision,
        selected.intentRevision,
        selected.backendGeneration,
        95);
    assert(provisioning.status ==
        TimerAssignmentFulfillmentStatus::provisioningStarted);
    const TimerAssignment assignment = provisioning.assignment;

    NativeTimerCreateOperationPayload payload;
    payload.timerAssignmentId = assignment.timerAssignmentId;
    payload.expectedAssignmentRevision = assignment.assignmentRevision;
    payload.expectedIntentRevision = assignment.intentRevision;
    payload.assignmentEpoch = assignment.assignmentEpoch;
    payload.nativeTimerBindingId = "binding:phase69:reconciliation";
    payload.backendId = assignment.backendId;
    payload.backendGeneration = assignment.backendGeneration;
    payload.expectedSpecification = specification();

    MutationOperation operation;
    operation.operationId = "operation:phase69:reconciliation";
    operation.idempotencyKey = "idempotency:phase69:reconciliation";
    operation.actorId = "actor:phase69";
    operation.backendId = assignment.backendId;
    operation.backendGeneration = assignment.backendGeneration;
    operation.resourceType = "TimerAssignment";
    operation.resourceId = assignment.timerAssignmentId;
    operation.expectedRevision = assignment.assignmentRevision;
    operation.expectedResourceFingerprint =
        nativeTimerSpecificationFingerprint(specification());
    operation.actionFamily = "timer.create";
    operation.requestFingerprint = "request:phase69:reconciliation";
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
    assert(reserved.operation.operationRevision == "1");

    NativeTimerCreateDispatchClaimRequest claim;
    claim.operationId = operation.operationId;
    claim.expectedOperationRevision = reserved.operation.operationRevision;
    claim.timerAssignmentId = assignment.timerAssignmentId;
    claim.nativeTimerBindingId = payload.nativeTimerBindingId;
    claim.backendId = assignment.backendId;
    claim.backendGeneration = assignment.backendGeneration;
    claim.expectedSpecificationFingerprint =
        operation.expectedResourceFingerprint;
    claim.reservation.commandId = "command:phase69:reconciliation";
    claim.reservation.requestFingerprint =
        "fingerprint:phase69:reconciliation";

    NativeTimerCreateDispatchService dispatch(operations);
    const auto claimed = dispatch.claimAfterReservation(claim, 110);
    assert(claimed.status == NativeTimerCreateDispatchClaimStatus::claimed);
    assert(claimed.operation.operationRevision == "2");

    NativeTimerCreateExecutorOutcome outcome;
    outcome.operationId = operation.operationId;
    outcome.operationRevision = claimed.operation.operationRevision;
    outcome.reservation = claim.reservation;
    outcome.category = NativeTimerCreateExecutorOutcomeCategory::acceptedUnverified;
    outcome.dispatchStartedAt = 124;
    outcome.completedAt = 130;
    outcome.evidenceReference = "suitebridge:ntcreate:phase69-reconciliation";

    const auto applied = dispatch.applyOutcome(outcome);
    assert(applied.status == NativeTimerCreateDispatchOutcomeStatus::applied);
    assert(applied.expectationPresent);
    assert(applied.expectation.readbackNotBefore == 124);

    const auto afterOutcome = operations.findById(operation.operationId);
    assert(afterOutcome.ok());
    assert(afterOutcome.operation.state ==
        MutationOperationState::executedUnverified);
    assert(afterOutcome.operation.operationRevision == "3");

    NativeTimerCreateReadbackVerificationService verification(bindings);
    NativeTimerCreateOperationCompletionService completion(
        operations, assignments, bindings);

    const auto staleEvidence =
        readbackEvidence(assignment, payload.nativeTimerBindingId, 123);
    const auto stale = reconcileNativeTimerCreateReadback(
        operations,
        verification,
        fulfillment,
        completion,
        applied.expectation,
        staleEvidence,
        140);
    assert(stale.status ==
        NativeTimerCreateReadbackReconciliationStatus::
            readbackVerificationFailed);
    assert(stale.verification.status ==
        NativeTimerCreateReadbackVerificationStatus::staleEvidence);

    const auto afterStale = operations.findById(operation.operationId);
    assert(afterStale.ok());
    assert(afterStale.operation.state ==
        MutationOperationState::executedUnverified);
    assert(afterStale.operation.operationRevision == "3");
    const auto stillProvisioning =
        assignments.findById(assignment.timerAssignmentId);
    assert(stillProvisioning.ok());
    assert(stillProvisioning.assignment.state ==
        TimerAssignmentState::provisioning);
    assert(bindings.findById(payload.nativeTimerBindingId).status ==
        NativeTimerBindingRepositoryStatus::notFound);

    const auto replayedOutcome = dispatch.applyOutcome(outcome);
    assert(replayedOutcome.status ==
        NativeTimerCreateDispatchOutcomeStatus::alreadyApplied);
    assert(replayedOutcome.expectationPresent);

    const auto evidence =
        readbackEvidence(assignment, payload.nativeTimerBindingId, 135);
    const auto reconciled = reconcileNativeTimerCreateReadback(
        operations,
        verification,
        fulfillment,
        completion,
        replayedOutcome.expectation,
        evidence,
        140);
    assert(reconciled.status ==
        NativeTimerCreateReadbackReconciliationStatus::completed);
    assert(reconciled.verification.status ==
        NativeTimerCreateReadbackVerificationStatus::verified);
    assert(reconciled.fulfillment.status ==
        TimerAssignmentFulfillmentStatus::bound);
    assert(reconciled.completion.status ==
        NativeTimerCreateOperationCompletionStatus::completed);

    const auto completedOperation = operations.findById(operation.operationId);
    assert(completedOperation.ok());
    assert(completedOperation.operation.state == MutationOperationState::succeeded);
    assert(completedOperation.operation.operationRevision == "4");

    const auto boundAssignment =
        assignments.findById(assignment.timerAssignmentId);
    assert(boundAssignment.ok());
    assert(boundAssignment.assignment.state == TimerAssignmentState::bound);
    assert(boundAssignment.assignment.nativeTimerBindingId ==
        payload.nativeTimerBindingId);

    const auto replay = reconcileNativeTimerCreateReadback(
        operations,
        verification,
        fulfillment,
        completion,
        replayedOutcome.expectation,
        evidence,
        140);
    assert(replay.status ==
        NativeTimerCreateReadbackReconciliationStatus::alreadyCompleted);
    assert(replay.verification.status ==
        NativeTimerCreateReadbackVerificationStatus::alreadyVerified);
    assert(replay.fulfillment.status ==
        TimerAssignmentFulfillmentStatus::alreadyBound);
    assert(replay.completion.status ==
        NativeTimerCreateOperationCompletionStatus::alreadyCompleted);

    const auto afterReplay = operations.findById(operation.operationId);
    assert(afterReplay.ok());
    assert(afterReplay.operation.operationRevision == "4");

    std::cout
        << "test_native_timer_create_readback_reconciliation passed\n";
    return 0;
}
