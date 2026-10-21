#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerModifyDispatchService.h"
#include "NativeTimerModifyOperationCompletionService.h"
#include "NativeTimerModifyReadbackVerificationService.h"
#include "NativeTimerSpecification.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification(
    NativeTimerModifyKind kind,
    bool enabled = true)
{
    NativeTimerSpecification value;
    value.channelId = "S19.2E-1-1019-10301";
    value.title = kind == NativeTimerModifyKind::update
        ? "Updated managed Timer" : "Managed Timer";
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

NativeTimerObservedState observedState(
    const NativeTimerSpecification& specification)
{
    NativeTimerObservedState value;
    value.channelId = specification.channelId;
    value.title = specification.title;
    value.directory = specification.directory;
    value.day = specification.day;
    value.weekdays = specification.weekdays;
    value.startTime = specification.startTime;
    value.endTime = specification.endTime;
    value.flags = specification.enabled ? 1 : 0;
    value.priority = specification.priority;
    value.lifetime = specification.lifetime;
    value.enabled = specification.enabled;
    value.vps = specification.vps;
    return value;
}

NativeTimerBinding createBinding(
    NativeTimerBindingRepository& repository,
    const std::string& suffix)
{
    NativeTimerBinding binding;
    binding.nativeTimerBindingId = "binding:modify:" + suffix;
    binding.backendId = "backend:1";
    binding.backendGeneration = 7;
    binding.backendNativeTimerId = "native:" + suffix;
    binding.timerAssignmentId = "assignment:modify:" + suffix;
    binding.ownership = NativeTimerBindingOwnership::managed;
    const auto before = specification(
        NativeTimerModifyKind::toggle, true);
    binding.observedState = observedState(before);
    binding.observedFingerprint =
        nativeTimerObservedStateFingerprint(binding.observedState);
    binding.lastObservedAt = 50;
    binding.lastVerifiedOperationId = "operation:create:" + suffix;
    const auto created = repository.create(binding);
    assert(created.ok());
    return created.binding;
}

NativeTimerModifyOperationPayload payloadFor(
    const NativeTimerBinding& binding,
    NativeTimerModifyKind kind)
{
    NativeTimerModifyOperationPayload payload;
    payload.kind = kind;
    payload.timerAssignmentId = binding.timerAssignmentId;
    payload.expectedAssignmentRevision = "assignment-revision:1";
    payload.expectedIntentRevision = "intent-revision:1";
    payload.assignmentEpoch = 1;
    payload.nativeTimerBindingId = binding.nativeTimerBindingId;
    payload.expectedBindingRevision = binding.bindingRevision;
    payload.backendId = binding.backendId;
    payload.backendGeneration = binding.backendGeneration;
    payload.backendNativeTimerId = binding.backendNativeTimerId;
    payload.expectedCurrentFingerprint = binding.observedFingerprint;
    payload.expectedSpecification = specification(
        kind, kind == NativeTimerModifyKind::toggle ? false : true);
    return payload;
}

MutationOperation reserve(
    MutationOperationRepository& repository,
    const NativeTimerModifyOperationPayload& payload,
    const std::string& suffix,
    std::int64_t requestedAt)
{
    MutationOperation operation;
    operation.operationId = "operation:modify:" + suffix;
    operation.idempotencyKey = "idempotency:modify:" + suffix;
    operation.actorId = "actor:owner";
    operation.backendId = payload.backendId;
    operation.backendGeneration = payload.backendGeneration;
    operation.resourceType = "NativeTimerBinding";
    operation.resourceId = payload.nativeTimerBindingId;
    operation.expectedRevision = payload.expectedBindingRevision;
    operation.expectedResourceFingerprint =
        payload.expectedCurrentFingerprint;
    operation.actionFamily =
        payload.kind == NativeTimerModifyKind::update
        ? "timer.update" : "timer.toggle";
    operation.requestFingerprint = "request:modify:" + suffix;
    operation.requestedAt = requestedAt;
    operation.deadline = requestedAt + 600;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    operation.state = MutationOperationState::accepted;
    operation.updatedAt = requestedAt;

    MutationOperationPayload durable;
    durable.operationId = operation.operationId;
    durable.payloadType = "native.timer.modify";
    durable.payloadVersion = 1;
    durable.payload = serializeNativeTimerModifyOperationPayload(payload);
    durable.payloadFingerprint =
        nativeTimerModifyOperationPayloadFingerprint(payload);
    const auto reserved = repository.reserveWithPayload(operation, durable);
    assert(reserved.ok());
    return reserved.operation;
}

void verifyRoundTrip(
    MutationOperationRepository& operations,
    NativeTimerBindingRepository& bindings,
    NativeTimerModifyKind kind,
    NativeTimerModifyExecutorOutcomeCategory category,
    const std::string& suffix)
{
    const NativeTimerBinding binding = createBinding(bindings, suffix);
    const auto payload = payloadFor(binding, kind);
    const MutationOperation operation =
        reserve(operations, payload, suffix, 100);

    NativeTimerModifyDispatchService dispatch(operations, bindings);
    NativeTimerModifyDispatchClaimRequest request;
    request.operationId = operation.operationId;
    request.expectedOperationRevision = operation.operationRevision;

    const auto claimed = dispatch.claim(request, 105);
    if (claimed.status != NativeTimerModifyDispatchClaimStatus::claimed)
        std::cerr << "modify claim failed: "
                  << static_cast<int>(claimed.status) << '\n';
    assert(claimed.status ==
        NativeTimerModifyDispatchClaimStatus::claimed);
    assert(claimed.operation.state == MutationOperationState::dispatching);
    assert(claimed.claim.payload.nativeTimerBindingId ==
        binding.nativeTimerBindingId);
    assert(dispatch.claim(request, 106).status ==
        NativeTimerModifyDispatchClaimStatus::alreadyClaimed);

    NativeTimerModifyExecutorOutcome outcome;
    outcome.category = category;
    outcome.dispatchStartedAt = 110;
    outcome.completedAt = 120;
    outcome.evidenceReference = "suitebridge:ntmod:" + suffix;
    const auto applied = dispatch.applyOutcome(claimed.claim, outcome);
    assert(applied.status ==
        NativeTimerModifyDispatchOutcomeStatus::applied);
    assert(applied.expectationPresent);
    assert(applied.expectation.readbackNotBefore == 110);
    assert(applied.expectation.operationState ==
        (category ==
            NativeTimerModifyExecutorOutcomeCategory::acceptedUnverified
        ? NativeTimerReadbackOperationState::executedUnverified
        : NativeTimerReadbackOperationState::outcomeUnknown));

    NativeTimerObservation observation;
    observation.backendId = binding.backendId;
    observation.backendGeneration = binding.backendGeneration;
    observation.backendNativeTimerId = binding.backendNativeTimerId;
    observation.observedAt = 130;
    observation.observedState =
        observedState(payload.expectedSpecification);
    observation.observedFingerprint =
        nativeTimerObservedStateFingerprint(observation.observedState);

    NativeTimerModifyReadbackVerificationService verification(bindings);
    const auto verified =
        verification.verify(applied.expectation, observation);
    assert(verified.status ==
        NativeTimerModifyReadbackVerificationStatus::verified);
    assert(verified.binding.lastVerifiedOperationId ==
        operation.operationId);

    NativeTimerModifyOperationCompletionService completion(
        operations, bindings);
    const auto completed = completion.complete(applied.expectation, 140);
    assert(completed.status ==
        NativeTimerModifyOperationCompletionStatus::completed);
    assert(completed.operation.state == MutationOperationState::succeeded);
    assert(completion.complete(applied.expectation, 140).status ==
        NativeTimerModifyOperationCompletionStatus::alreadyCompleted);
}

void verifyRejectedWithoutEffect(
    MutationOperationRepository& operations,
    NativeTimerBindingRepository& bindings)
{
    const NativeTimerBinding binding =
        createBinding(bindings, "rejected");
    const auto payload = payloadFor(
        binding, NativeTimerModifyKind::update);
    const MutationOperation operation =
        reserve(operations, payload, "rejected", 200);

    NativeTimerModifyDispatchService dispatch(operations, bindings);
    NativeTimerModifyDispatchClaimRequest request;
    request.operationId = operation.operationId;
    request.expectedOperationRevision = operation.operationRevision;
    const auto claimed = dispatch.claim(request, 205);
    assert(claimed.ok());

    NativeTimerModifyExecutorOutcome outcome;
    outcome.category =
        NativeTimerModifyExecutorOutcomeCategory::rejectedWithoutEffect;
    outcome.completedAt = 210;
    outcome.evidenceReference = "suitebridge:ntmod:rejected";
    const auto applied = dispatch.applyOutcome(claimed.claim, outcome);
    assert(applied.ok());
    assert(!applied.expectationPresent);
    assert(applied.operation.state ==
        MutationOperationState::failedVerified);
}
void verifyEvidenceFreeLegacyQuarantine(
    MutationOperationRepository& operations,
    NativeTimerBindingRepository& bindings)
{
    const auto binding = createBinding(bindings, "missing-evidence");
    const auto payload = payloadFor(binding, NativeTimerModifyKind::update);
    const auto operation = reserve(operations, payload, "missing-evidence", 300);
    NativeTimerModifyDispatchService dispatch(operations, bindings);
    NativeTimerModifyDispatchClaimRequest claim;
    claim.operationId = operation.operationId;
    claim.expectedOperationRevision = operation.operationRevision;
    const auto claimed = dispatch.claim(claim, 305);
    assert(claimed.ok());
    assert(claimed.operation.state == MutationOperationState::dispatching);

    // A persisted legacy ACK lacking evidence can only be quarantined. It
    // cannot establish either success or verified no-effect, and it does not
    // authorize another native dispatch.
    const std::string reason =
        "native-timer-modify:missing-result-evidence";
    const auto quarantined = operations.transition(
        operation.operationId, claimed.operation.operationRevision,
        MutationOperationState::dispatching,
        MutationOperationState::outcomeUnknown, reason, 310);
    assert(quarantined.ok());
    assert(quarantined.operation.state == MutationOperationState::outcomeUnknown);
    assert(quarantined.operation.resultReference == reason);
    const auto persisted = operations.findById(operation.operationId);
    assert(persisted.ok());
    assert(persisted.operation.state == MutationOperationState::outcomeUnknown);
    const auto staleSuccess = operations.transition(
        operation.operationId, claimed.operation.operationRevision,
        MutationOperationState::outcomeUnknown,
        MutationOperationState::succeeded, "unverified-success", 311);
    assert(!staleSuccess.ok());
}

void verifyUncertainNoEffectReconcilesExactlyOnce(
    MutationOperationRepository& operations,
    NativeTimerBindingRepository& bindings)
{
    const auto binding = createBinding(bindings, "uncertain-no-effect");
    const auto payload = payloadFor(binding, NativeTimerModifyKind::update);
    const auto operation = reserve(
        operations, payload, "uncertain-no-effect", 400);
    NativeTimerModifyDispatchService dispatch(operations, bindings);
    NativeTimerModifyDispatchClaimRequest request;
    request.operationId = operation.operationId;
    request.expectedOperationRevision = operation.operationRevision;
    const auto claimed = dispatch.claim(request, 405);
    assert(claimed.ok());

    NativeTimerModifyExecutorOutcome outcome;
    outcome.category = NativeTimerModifyExecutorOutcomeCategory::outcomeUnknown;
    outcome.dispatchStartedAt = 410;
    outcome.completedAt = 411;
    outcome.evidenceReference = "suitebridge:ntmod:transport-outcome-unknown";
    const auto applied = dispatch.applyOutcome(claimed.claim, outcome);
    assert(applied.status == NativeTimerModifyDispatchOutcomeStatus::applied);
    assert(applied.operation.state == MutationOperationState::outcomeUnknown);
    assert(applied.expectationPresent);

    const auto repeated = dispatch.applyOutcome(claimed.claim, outcome);
    assert(repeated.status ==
        NativeTimerModifyDispatchOutcomeStatus::alreadyApplied);
    assert(repeated.operation.operationRevision ==
        applied.operation.operationRevision);

    NativeTimerObservation observation;
    observation.backendId = binding.backendId;
    observation.backendGeneration = binding.backendGeneration;
    observation.backendNativeTimerId = binding.backendNativeTimerId;
    observation.observedAt = 420;
    observation.observedState = binding.observedState;
    observation.observedFingerprint =
        nativeTimerObservedStateFingerprint(observation.observedState);
    assert(nativeTimerObservationValid(observation));
    assert(observation.observedFingerprint == payload.expectedCurrentFingerprint);
    assert(!nativeTimerObservationMatchesSpecification(
        payload.expectedSpecification, observation.observedState));

    // A fresh matching original-state readback closes the persisted unknown
    // attempt without issuing a second native modification.
    const auto closed = operations.transition(
        operation.operationId, applied.operation.operationRevision,
        MutationOperationState::outcomeUnknown,
        MutationOperationState::failedVerified,
        "native-timer-modify:unchanged-after-uncertain-execution", 421);
    assert(closed.ok());
    assert(closed.operation.state == MutationOperationState::failedVerified);
    assert(bindings.findById(binding.nativeTimerBindingId).binding
        .observedState.title == binding.observedState.title);
}

void verifyEventIdOnlyDriftNoEffectFence()
{
    // A previous RESTfulAPI null event_id was accidentally read as the
    // following field name, "weekdays". The later EPG association must not
    // obscure proof that all timer-writing fields are unchanged.
    NativeTimerObservedState before = observedState(
        specification(NativeTimerModifyKind::toggle, true));
    before.eventId = "weekdays";
    NativeTimerObservedState live = before;
    live.eventId = "13546";
    assert(nativeTimerObservedStateFingerprint(before) !=
           nativeTimerObservedStateFingerprint(live));
    assert(nativeTimerObservedStateSameExceptEventId(before, live));

    // Title/clock/channel/flags/recording/pending changes must still fence
    // no-effect resolution even when the EPG event identity differs.
    live.title = "Updated managed Timer";
    assert(!nativeTimerObservedStateSameExceptEventId(before, live));
    live = before;
    live.eventId = "13546";
    live.startTime = "0945";
    assert(!nativeTimerObservedStateSameExceptEventId(before, live));
    live = before;
    live.eventId = "13546";
    live.flags = 0;
    assert(!nativeTimerObservedStateSameExceptEventId(before, live));
    live = before;
    live.eventId = "13546";
    live.recording = true;
    assert(!nativeTimerObservedStateSameExceptEventId(before, live));
    live = before;
    live.eventId = "13546";
    live.pending = true;
    assert(!nativeTimerObservedStateSameExceptEventId(before, live));
}

void verifyPendingBacklogBeyondFirstPage(
    MutationOperationRepository& operations,
    NativeTimerBindingRepository& bindings)
{
    for (int index = 0; index < 40; ++index)
    {
        const std::string suffix = "backlog-" + std::to_string(index);
        const auto binding = createBinding(bindings, suffix);
        const auto payload = payloadFor(binding, NativeTimerModifyKind::update);
        (void)reserve(operations, payload, suffix, 500 + index);
    }
    const auto pending = operations.listByActionFamilyAndStates(
        "timer.update", {MutationOperationState::accepted}, 256);
    assert(pending.ok());
    assert(pending.operations.size() >= 40U);
}

} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));

    MutationOperationRepository operations(database);
    NativeTimerBindingRepository bindings(database);
    assert(operations.ensureSchema());
    assert(bindings.ensureSchema());

    verifyRoundTrip(
        operations,
        bindings,
        NativeTimerModifyKind::update,
        NativeTimerModifyExecutorOutcomeCategory::acceptedUnverified,
        "update");
    verifyRoundTrip(
        operations,
        bindings,
        NativeTimerModifyKind::toggle,
        NativeTimerModifyExecutorOutcomeCategory::outcomeUnknown,
        "toggle");
    verifyRejectedWithoutEffect(operations, bindings);
    verifyEvidenceFreeLegacyQuarantine(operations, bindings);
    verifyUncertainNoEffectReconcilesExactlyOnce(operations, bindings);
    verifyEventIdOnlyDriftNoEffectFence();
    verifyPendingBacklogBeyondFirstPage(operations, bindings);

    std::cout << "test_native_timer_modify_dispatch passed\n";
    return 0;
}
