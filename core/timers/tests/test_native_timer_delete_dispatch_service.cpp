#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBinding.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerDeleteDispatchService.h"
#include "NativeTimerDeleteOperationPreparationService.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerObservedState observedState()
{
    NativeTimerObservedState state;
    state.channelId = "channel:1";
    state.weekdays = "-------";
    state.startTime = "930";
    state.endTime = "1000";
    return state;
}

NativeTimerBinding binding(const std::string& id, const std::string& nativeId)
{
    NativeTimerBinding value;
    value.nativeTimerBindingId = id;
    value.backendId = "backend:a";
    value.backendGeneration = 7;
    value.backendNativeTimerId = nativeId;
    value.timerAssignmentId = "assignment:" + id;
    value.ownership = NativeTimerBindingOwnership::managed;
    value.observedState = observedState();
    value.observedFingerprint = nativeTimerObservedStateFingerprint(value.observedState);
    value.lastObservedAt = 1900;
    return value;
}

NativeTimerDeleteOperationPreparationRequest requestFor(
    const NativeTimerBinding& binding,
    const std::string& operationId,
    const std::string& idempotencyKey)
{
    NativeTimerDeleteOperationPreparationRequest request;
    request.operationId = operationId;
    request.idempotencyKey = idempotencyKey;
    request.actorId = "actor:1";
    request.requestFingerprint = "sha256:" + operationId;
    request.nativeTimerBindingId = binding.nativeTimerBindingId;
    request.expectedBindingRevision = binding.bindingRevision;
    request.expectedBackendGeneration = binding.backendGeneration;
    request.requestedAt = 2000;
    request.deadline = 4000;
    return request;
}

NativeTimerDeleteDispatchHandoff preparedHandoff(
    MutationOperationRepository& operations,
    NativeTimerBindingRepository& bindings,
    const NativeTimerBinding& binding,
    const std::string& operationId,
    const std::string& idempotencyKey)
{
    NativeTimerDeleteOperationPreparationService preparation(operations, bindings);
    const auto prepared = preparation.prepare(
        requestFor(binding, operationId, idempotencyKey));
    assert(prepared.status == NativeTimerDeleteOperationPreparationStatus::prepared);
    return prepared.handoff;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository operations(database);
    NativeTimerBindingRepository bindings(database);
    assert(operations.ensureSchema());
    assert(bindings.ensureSchema());

    NativeTimerDeleteDispatchService service(operations, bindings);

    const auto created = bindings.create(binding("binding:1", "timer:17"));
    assert(created.ok());
    const auto handoff = preparedHandoff(
        operations, bindings, created.binding,
        "operation:delete:1", "idem:delete:1");
    assert(handoff.expectedNativeTimerFingerprint ==
        created.binding.observedFingerprint);

    const auto claimed = service.claim(handoff, 2100);
    assert(claimed.status == NativeTimerDeleteDispatchClaimStatus::claimed);
    assert(claimed.operation.state == MutationOperationState::dispatching);
    assert(claimed.operation.operationRevision == "2");
    assert(claimed.claim.operationRevision == "2");
    assert(claimed.claim.expectedNativeTimerFingerprint ==
        handoff.expectedNativeTimerFingerprint);
    assert(claimed.claim.claimedAt == 2100);

    const auto claimReplay = service.claim(handoff, 2200);
    assert(claimReplay.status == NativeTimerDeleteDispatchClaimStatus::alreadyClaimed);
    assert(claimReplay.claim.operationRevision == "2");
    assert(claimReplay.claim.expectedNativeTimerFingerprint ==
        handoff.expectedNativeTimerFingerprint);
    assert(claimReplay.claim.claimedAt == 2100);

    NativeTimerDeleteExecutorOutcome accepted;
    accepted.category = NativeTimerDeleteExecutorOutcomeCategory::acceptedUnverified;
    accepted.dispatchStartedAt = 2150;
    accepted.completedAt = 2160;
    accepted.evidenceReference = "agent-command:cmd-1";
    const auto acceptedResult = service.applyOutcome(claimed.claim, accepted);
    assert(acceptedResult.status == NativeTimerDeleteDispatchOutcomeStatus::applied);
    assert(acceptedResult.operation.state == MutationOperationState::executedUnverified);
    assert(acceptedResult.operation.operationRevision == "3");
    assert(acceptedResult.expectationPresent);
    assert(acceptedResult.expectation.operationId == handoff.operationId);
    assert(acceptedResult.expectation.operationState ==
        NativeTimerReadbackOperationState::executedUnverified);
    assert(acceptedResult.expectation.readbackNotBefore == 2150);
    assert(acceptedResult.expectation.expectedBindingRevision ==
        handoff.expectedBindingRevision);

    const auto acceptedReplay = service.applyOutcome(claimed.claim, accepted);
    assert(acceptedReplay.status == NativeTimerDeleteDispatchOutcomeStatus::alreadyApplied);
    assert(acceptedReplay.expectationPresent);
    assert(acceptedReplay.expectation.readbackNotBefore == 2150);

    const auto unknownCreated = bindings.create(
        binding("binding:unknown", "timer:unknown"));
    assert(unknownCreated.ok());
    const auto unknownHandoff = preparedHandoff(
        operations, bindings, unknownCreated.binding,
        "operation:delete:unknown", "idem:delete:unknown");
    const auto unknownClaim = service.claim(unknownHandoff, 3100);
    assert(unknownClaim.ok());
    NativeTimerDeleteExecutorOutcome unknown;
    unknown.category = NativeTimerDeleteExecutorOutcomeCategory::outcomeUnknown;
    unknown.dispatchStartedAt = 3150;
    unknown.completedAt = 3200;
    unknown.evidenceReference = "agent-command:cmd-unknown";
    const auto unknownResult = service.applyOutcome(unknownClaim.claim, unknown);
    assert(unknownResult.status == NativeTimerDeleteDispatchOutcomeStatus::applied);
    assert(unknownResult.operation.state == MutationOperationState::outcomeUnknown);
    assert(unknownResult.expectationPresent);
    assert(unknownResult.expectation.operationState ==
        NativeTimerReadbackOperationState::outcomeUnknown);
    assert(unknownResult.expectation.readbackNotBefore == 3150);

    const auto rejectedCreated = bindings.create(
        binding("binding:rejected", "timer:rejected"));
    assert(rejectedCreated.ok());
    const auto rejectedHandoff = preparedHandoff(
        operations, bindings, rejectedCreated.binding,
        "operation:delete:rejected", "idem:delete:rejected");
    const auto rejectedClaim = service.claim(rejectedHandoff, 4100);
    assert(rejectedClaim.ok());
    NativeTimerDeleteExecutorOutcome rejected;
    rejected.category = NativeTimerDeleteExecutorOutcomeCategory::rejectedWithoutEffect;
    rejected.dispatchStartedAt = 0;
    rejected.completedAt = 4120;
    rejected.evidenceReference = "agent-command:rejected-before-effect";
    const auto rejectedResult = service.applyOutcome(rejectedClaim.claim, rejected);
    assert(rejectedResult.status == NativeTimerDeleteDispatchOutcomeStatus::applied);
    assert(rejectedResult.operation.state == MutationOperationState::failedVerified);
    assert(!rejectedResult.expectationPresent);

    NativeTimerDeleteExecutorOutcome badFence = accepted;
    badFence.dispatchStartedAt = claimed.claim.claimedAt - 1;
    assert(service.applyOutcome(claimed.claim, badFence).status ==
        NativeTimerDeleteDispatchOutcomeStatus::invalid);

    const auto staleCreated = bindings.create(
        binding("binding:stale", "timer:stale"));
    assert(staleCreated.ok());
    const auto staleHandoff = preparedHandoff(
        operations, bindings, staleCreated.binding,
        "operation:delete:stale", "idem:delete:stale");
    NativeTimerBinding staleBinding = staleCreated.binding;
    staleBinding.lastObservedAt = 1950;
    const auto staleUpdated = bindings.update(
        staleBinding, staleCreated.binding.bindingRevision);
    assert(staleUpdated.ok());
    assert(service.claim(staleHandoff, 5100).status ==
        NativeTimerDeleteDispatchClaimStatus::bindingRevisionConflict);

    const auto fingerprintCreated = bindings.create(
        binding("binding:fingerprint-race", "timer:fingerprint-race"));
    assert(fingerprintCreated.ok());
    const auto fingerprintHandoff = preparedHandoff(
        operations, bindings, fingerprintCreated.binding,
        "operation:delete:fingerprint-race", "idem:delete:fingerprint-race");
    NativeTimerObservedState changedState = fingerprintCreated.binding.observedState;
    changedState.startTime = "945";
    const std::string changedFingerprint =
        nativeTimerObservedStateFingerprint(changedState);
    assert(!changedFingerprint.empty());
    assert(changedFingerprint != fingerprintHandoff.expectedNativeTimerFingerprint);
    const std::string mutateObservedState =
        "UPDATE native_timer_bindings SET observed_start_time='945',"
        "observed_fingerprint='" + changedFingerprint +
        "' WHERE native_timer_binding_id='binding:fingerprint-race';";
    assert(database.execute(mutateObservedState));
    const auto fingerprintCurrent =
        bindings.findById(fingerprintCreated.binding.nativeTimerBindingId);
    assert(fingerprintCurrent.ok());
    assert(fingerprintCurrent.binding.bindingRevision ==
        fingerprintHandoff.expectedBindingRevision);
    assert(fingerprintCurrent.binding.observedFingerprint == changedFingerprint);
    assert(service.claim(fingerprintHandoff, 5600).status ==
        NativeTimerDeleteDispatchClaimStatus::bindingStateConflict);
    const auto fingerprintOperation =
        operations.findById(fingerprintHandoff.operationId);
    assert(fingerprintOperation.ok());
    assert(fingerprintOperation.operation.state == MutationOperationState::accepted);

    const auto tamperedCreated = bindings.create(
        binding("binding:tampered-fingerprint", "timer:tampered-fingerprint"));
    assert(tamperedCreated.ok());
    auto tamperedHandoff = preparedHandoff(
        operations, bindings, tamperedCreated.binding,
        "operation:delete:tampered-fingerprint",
        "idem:delete:tampered-fingerprint");
    tamperedHandoff.expectedNativeTimerFingerprint = changedFingerprint;
    assert(service.claim(tamperedHandoff, 5800).status ==
        NativeTimerDeleteDispatchClaimStatus::identityConflict);
    const auto tamperedOperation = operations.findById(tamperedHandoff.operationId);
    assert(tamperedOperation.ok());
    assert(tamperedOperation.operation.state == MutationOperationState::accepted);

    const auto driftCreated = bindings.create(
        binding("binding:drift", "timer:drift"));
    assert(driftCreated.ok());
    const auto driftHandoff = preparedHandoff(
        operations, bindings, driftCreated.binding,
        "operation:delete:drift", "idem:delete:drift");
    NativeTimerBinding drifted = driftCreated.binding;
    drifted.driftState = NativeTimerBindingDriftState::externalFieldChange;
    drifted.lastObservedAt = 1950;
    const auto driftUpdated = bindings.update(
        drifted, driftCreated.binding.bindingRevision);
    assert(driftUpdated.ok());
    auto currentDriftHandoff = driftHandoff;
    currentDriftHandoff.expectedBindingRevision = driftUpdated.binding.bindingRevision;
    auto driftOperation = operations.findById(driftHandoff.operationId);
    assert(driftOperation.ok());
    // The prepared operation is bound to the old revision, so the changed
    // handoff cannot be substituted and fails identity correlation first.
    assert(service.claim(currentDriftHandoff, 6100).status ==
        NativeTimerDeleteDispatchClaimStatus::identityConflict);

    auto wrongNative = unknownHandoff;
    wrongNative.operationId = "operation:delete:wrong-native";
    // Unknown operation is checked before binding identity.
    assert(service.claim(wrongNative, 7100).status ==
        NativeTimerDeleteDispatchClaimStatus::operationNotFound);

    auto invalidHandoff = unknownHandoff;
    invalidHandoff.expectedNativeTimerFingerprint.clear();
    assert(service.claim(invalidHandoff, 7200).status ==
        NativeTimerDeleteDispatchClaimStatus::invalid);

    NativeTimerDeleteDispatchClaim invalidClaim = claimed.claim;
    invalidClaim.operationRevision.clear();
    assert(service.applyOutcome(invalidClaim, accepted).status ==
        NativeTimerDeleteDispatchOutcomeStatus::invalid);

    invalidClaim = claimed.claim;
    invalidClaim.expectedNativeTimerFingerprint.clear();
    assert(service.applyOutcome(invalidClaim, accepted).status ==
        NativeTimerDeleteDispatchOutcomeStatus::invalid);

    std::cout << "Phase 64 native Timer delete dispatch contract regression passed\n";
    return 0;
}
