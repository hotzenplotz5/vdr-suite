#include "NativeTimerModifyDispatchService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"

#include <cstddef>

namespace vdrsuite::timers
{
namespace
{
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationPayload;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::operations::MutationOperationVerificationPolicy;

constexpr std::size_t MaxIdentity = 160;
constexpr std::size_t MaxEvidenceReference = 512;

bool identity(const std::string& value)
{
    return !value.empty() && value.size() <= MaxIdentity;
}

bool suiteManaged(NativeTimerBindingOwnership ownership)
{
    return ownership == NativeTimerBindingOwnership::managed ||
        ownership == NativeTimerBindingOwnership::adopted;
}

const char* actionFamily(NativeTimerModifyKind kind)
{
    return kind == NativeTimerModifyKind::update
        ? "timer.update" : "timer.toggle";
}

bool durablePayloadMatches(
    const MutationOperationPayload& durable,
    NativeTimerModifyOperationPayload& payload)
{
    if (durable.payloadType != "native.timer.modify" ||
        durable.payloadVersion != 1 ||
        !parseNativeTimerModifyOperationPayload(durable.payload, payload))
        return false;
    return durable.payloadFingerprint ==
        nativeTimerModifyOperationPayloadFingerprint(payload) &&
        durable.payload ==
            serializeNativeTimerModifyOperationPayload(payload);
}

bool operationMatchesPayload(
    const MutationOperation& operation,
    const NativeTimerModifyOperationPayload& payload)
{
    return operation.backendId == payload.backendId &&
        operation.backendGeneration == payload.backendGeneration &&
        operation.resourceType == "NativeTimerBinding" &&
        operation.resourceId == payload.nativeTimerBindingId &&
        operation.expectedRevision == payload.expectedBindingRevision &&
        operation.expectedResourceFingerprint ==
            payload.expectedCurrentFingerprint &&
        operation.actionFamily == actionFamily(payload.kind) &&
        operation.verificationPolicy ==
            MutationOperationVerificationPolicy::readbackRequired;
}

bool bindingMatchesPayload(
    const NativeTimerBinding& binding,
    const NativeTimerModifyOperationPayload& payload)
{
    return binding.nativeTimerBindingId == payload.nativeTimerBindingId &&
        binding.bindingRevision == payload.expectedBindingRevision &&
        binding.timerAssignmentId == payload.timerAssignmentId &&
        binding.backendId == payload.backendId &&
        binding.backendGeneration == payload.backendGeneration &&
        binding.backendNativeTimerId == payload.backendNativeTimerId;
}

bool bindingDispatchable(
    const NativeTimerBinding& binding,
    const NativeTimerModifyOperationPayload& payload)
{
    return bindingMatchesPayload(binding, payload) &&
        suiteManaged(binding.ownership) &&
        binding.observedFingerprint == payload.expectedCurrentFingerprint &&
        binding.missingSince == 0 &&
        binding.driftState == NativeTimerBindingDriftState::none &&
        !binding.observedState.recording &&
        !binding.observedState.pending;
}

NativeTimerModifyDispatchClaim claimFrom(
    const MutationOperation& operation,
    const NativeTimerModifyOperationPayload& payload,
    std::int64_t claimedAt)
{
    NativeTimerModifyDispatchClaim claim;
    claim.operationId = operation.operationId;
    claim.operationRevision = operation.operationRevision;
    claim.payload = payload;
    claim.claimedAt = claimedAt;
    return claim;
}

NativeTimerModifyDispatchClaimResult claimResult(
    NativeTimerModifyDispatchClaimStatus status,
    const MutationOperation& operation = {},
    const NativeTimerModifyDispatchClaim& claim = {})
{
    NativeTimerModifyDispatchClaimResult result;
    result.status = status;
    result.operation = operation;
    result.claim = claim;
    return result;
}

NativeTimerModifyDispatchOutcomeResult outcomeResult(
    NativeTimerModifyDispatchOutcomeStatus status,
    const MutationOperation& operation = {},
    const NativeTimerModifyReadbackExpectation* expectation = nullptr)
{
    NativeTimerModifyDispatchOutcomeResult result;
    result.status = status;
    result.operation = operation;
    if (expectation != nullptr)
    {
        result.expectationPresent = true;
        result.expectation = *expectation;
    }
    return result;
}

bool claimValid(const NativeTimerModifyDispatchClaim& claim)
{
    return identity(claim.operationId) &&
        identity(claim.operationRevision) &&
        nativeTimerModifyOperationPayloadValid(claim.payload) &&
        claim.claimedAt > 0;
}

MutationOperationState outcomeState(
    NativeTimerModifyExecutorOutcomeCategory category)
{
    switch (category)
    {
        case NativeTimerModifyExecutorOutcomeCategory::rejectedWithoutEffect:
            return MutationOperationState::failedVerified;
        case NativeTimerModifyExecutorOutcomeCategory::acceptedUnverified:
            return MutationOperationState::executedUnverified;
        case NativeTimerModifyExecutorOutcomeCategory::outcomeUnknown:
            return MutationOperationState::outcomeUnknown;
    }
    return MutationOperationState::outcomeUnknown;
}

bool outcomeValid(
    const NativeTimerModifyDispatchClaim& claim,
    const NativeTimerModifyExecutorOutcome& outcome)
{
    if (!claimValid(claim) ||
        outcome.completedAt < claim.claimedAt ||
        outcome.evidenceReference.empty() ||
        outcome.evidenceReference.size() > MaxEvidenceReference)
        return false;

    switch (outcome.category)
    {
        case NativeTimerModifyExecutorOutcomeCategory::rejectedWithoutEffect:
            return outcome.dispatchStartedAt == 0;
        case NativeTimerModifyExecutorOutcomeCategory::acceptedUnverified:
        case NativeTimerModifyExecutorOutcomeCategory::outcomeUnknown:
            return outcome.dispatchStartedAt >= claim.claimedAt &&
                outcome.dispatchStartedAt <= outcome.completedAt;
    }
    return false;
}

NativeTimerModifyReadbackExpectation expectationFor(
    const NativeTimerModifyDispatchClaim& claim,
    const NativeTimerModifyExecutorOutcome& outcome)
{
    NativeTimerModifyReadbackExpectation expectation;
    expectation.operationId = claim.operationId;
    expectation.operationState =
        outcome.category ==
            NativeTimerModifyExecutorOutcomeCategory::acceptedUnverified
        ? NativeTimerReadbackOperationState::executedUnverified
        : NativeTimerReadbackOperationState::outcomeUnknown;
    expectation.payload = claim.payload;
    expectation.readbackNotBefore = outcome.dispatchStartedAt;
    return expectation;
}
} // namespace

NativeTimerModifyDispatchService::NativeTimerModifyDispatchService(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    NativeTimerBindingRepository& bindingRepository)
    : operationRepository_(operationRepository),
      bindingRepository_(bindingRepository)
{
}

NativeTimerModifyDispatchClaimResult NativeTimerModifyDispatchService::claim(
    const NativeTimerModifyDispatchClaimRequest& request,
    std::int64_t claimedAt)
{
    if (!identity(request.operationId) ||
        !identity(request.expectedOperationRevision) ||
        claimedAt <= 0)
        return claimResult(NativeTimerModifyDispatchClaimStatus::invalid);

    const auto foundOperation =
        operationRepository_.findById(request.operationId);
    if (foundOperation.status == MutationOperationRepositoryStatus::notFound)
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::operationNotFound);
    if (!foundOperation.ok())
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::operationRepositoryError);

    const MutationOperation& operation = foundOperation.operation;
    if (operation.state != MutationOperationState::accepted &&
        operation.state != MutationOperationState::dispatching)
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::operationStateConflict,
            operation);
    if (operation.state == MutationOperationState::accepted &&
        operation.operationRevision != request.expectedOperationRevision)
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::operationRevisionConflict,
            operation);

    const auto foundPayload =
        operationRepository_.findPayloadByOperationId(request.operationId);
    if (foundPayload.status == MutationOperationRepositoryStatus::notFound)
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::payloadNotFound,
            operation);
    if (!foundPayload.ok())
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::operationRepositoryError,
            operation);

    NativeTimerModifyOperationPayload payload;
    if (!durablePayloadMatches(foundPayload.payload, payload))
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::payloadConflict,
            operation);
    if (!operationMatchesPayload(operation, payload))
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::identityConflict,
            operation);

    const auto foundBinding =
        bindingRepository_.findById(payload.nativeTimerBindingId);
    if (foundBinding.status == NativeTimerBindingRepositoryStatus::notFound)
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::bindingNotFound,
            operation);
    if (!foundBinding.ok())
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::bindingRepositoryError,
            operation);

    const NativeTimerBinding& binding = foundBinding.binding;
    if (!bindingMatchesPayload(binding, payload))
    {
        if (binding.bindingRevision != payload.expectedBindingRevision)
            return claimResult(
                NativeTimerModifyDispatchClaimStatus::bindingRevisionConflict,
                operation);
        if (binding.backendGeneration != payload.backendGeneration)
            return claimResult(
                NativeTimerModifyDispatchClaimStatus::generationConflict,
                operation);
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::identityConflict,
            operation);
    }
    if (!suiteManaged(binding.ownership))
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::ownershipConflict,
            operation);
    if (!bindingDispatchable(binding, payload))
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::bindingStateConflict,
            operation);

    if (operation.state == MutationOperationState::dispatching)
    {
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::alreadyClaimed,
            operation,
            claimFrom(operation, payload, operation.updatedAt));
    }

    const auto transitioned = operationRepository_.transition(
        operation.operationId,
        operation.operationRevision,
        MutationOperationState::accepted,
        MutationOperationState::dispatching,
        "",
        claimedAt);
    if (transitioned.status == MutationOperationRepositoryStatus::ok)
    {
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::claimed,
            transitioned.operation,
            claimFrom(transitioned.operation, payload, claimedAt));
    }
    if (transitioned.status ==
        MutationOperationRepositoryStatus::idempotentReplay)
    {
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::alreadyClaimed,
            transitioned.operation,
            claimFrom(
                transitioned.operation,
                payload,
                transitioned.operation.updatedAt));
    }
    if (transitioned.status ==
        MutationOperationRepositoryStatus::revisionConflict)
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::operationRevisionConflict,
            transitioned.operation);
    if (transitioned.status ==
        MutationOperationRepositoryStatus::stateConflict)
        return claimResult(
            NativeTimerModifyDispatchClaimStatus::operationStateConflict,
            transitioned.operation);
    return claimResult(
        NativeTimerModifyDispatchClaimStatus::operationRepositoryError,
        transitioned.operation);
}

NativeTimerModifyDispatchOutcomeResult
NativeTimerModifyDispatchService::applyOutcome(
    const NativeTimerModifyDispatchClaim& claim,
    const NativeTimerModifyExecutorOutcome& outcome)
{
    if (!outcomeValid(claim, outcome))
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::invalid);

    const auto found = operationRepository_.findById(claim.operationId);
    if (found.status == MutationOperationRepositoryStatus::notFound)
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::operationNotFound);
    if (!found.ok())
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::operationRepositoryError);

    const MutationOperation& operation = found.operation;
    if (!operationMatchesPayload(operation, claim.payload))
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::identityConflict,
            operation);

    const MutationOperationState target = outcomeState(outcome.category);
    if (operation.state != MutationOperationState::dispatching &&
        operation.state != target)
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::operationStateConflict,
            operation);
    if (operation.state == MutationOperationState::dispatching &&
        operation.operationRevision != claim.operationRevision)
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::operationRevisionConflict,
            operation);

    NativeTimerModifyReadbackExpectation expectation;
    const bool needsReadback =
        outcome.category !=
            NativeTimerModifyExecutorOutcomeCategory::rejectedWithoutEffect;
    if (needsReadback)
    {
        expectation = expectationFor(claim, outcome);
        if (!nativeTimerModifyReadbackExpectationValid(expectation))
            return outcomeResult(
                NativeTimerModifyDispatchOutcomeStatus::invalid,
                operation);
    }

    const auto transitioned = operationRepository_.transition(
        operation.operationId,
        claim.operationRevision,
        MutationOperationState::dispatching,
        target,
        outcome.evidenceReference,
        outcome.completedAt);
    if (transitioned.status == MutationOperationRepositoryStatus::ok)
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::applied,
            transitioned.operation,
            needsReadback ? &expectation : nullptr);
    if (transitioned.status ==
        MutationOperationRepositoryStatus::idempotentReplay)
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::alreadyApplied,
            transitioned.operation,
            needsReadback ? &expectation : nullptr);
    if (transitioned.status ==
        MutationOperationRepositoryStatus::revisionConflict)
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::operationRevisionConflict,
            transitioned.operation);
    if (transitioned.status ==
        MutationOperationRepositoryStatus::stateConflict)
        return outcomeResult(
            NativeTimerModifyDispatchOutcomeStatus::operationStateConflict,
            transitioned.operation);
    return outcomeResult(
        NativeTimerModifyDispatchOutcomeStatus::operationRepositoryError,
        transitioned.operation);
}

} // namespace vdrsuite::timers
