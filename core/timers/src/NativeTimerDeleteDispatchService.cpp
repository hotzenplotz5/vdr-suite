#include "NativeTimerDeleteDispatchService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"

#include <cstddef>

namespace vdrsuite::timers
{
namespace
{
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::operations::MutationOperationVerificationPolicy;

constexpr std::size_t kMaxEvidenceReferenceLength = 512;

bool suiteManaged(NativeTimerBindingOwnership ownership)
{
    return ownership == NativeTimerBindingOwnership::managed ||
        ownership == NativeTimerBindingOwnership::adopted;
}

bool operationMatchesHandoff(
    const MutationOperation& operation,
    const NativeTimerDeleteDispatchHandoff& handoff)
{
    return operation.operationId == handoff.operationId &&
        operation.backendId == handoff.backendId &&
        operation.backendGeneration == handoff.backendGeneration &&
        operation.resourceType == "NativeTimerBinding" &&
        operation.resourceId == handoff.nativeTimerBindingId &&
        operation.expectedRevision == handoff.expectedBindingRevision &&
        operation.actionFamily == "timer.delete" &&
        operation.verificationPolicy ==
            MutationOperationVerificationPolicy::readbackRequired;
}

bool bindingMatchesHandoff(
    const NativeTimerBinding& binding,
    const NativeTimerDeleteDispatchHandoff& handoff)
{
    return binding.nativeTimerBindingId == handoff.nativeTimerBindingId &&
        binding.bindingRevision == handoff.expectedBindingRevision &&
        binding.timerAssignmentId == handoff.timerAssignmentId &&
        binding.backendId == handoff.backendId &&
        binding.backendGeneration == handoff.backendGeneration &&
        binding.backendNativeTimerId == handoff.backendNativeTimerId;
}

NativeTimerDeleteDispatchClaim claimFrom(
    const MutationOperation& operation,
    const NativeTimerDeleteDispatchHandoff& handoff,
    std::int64_t claimedAt)
{
    NativeTimerDeleteDispatchClaim claim;
    claim.operationId = operation.operationId;
    claim.operationRevision = operation.operationRevision;
    claim.nativeTimerBindingId = handoff.nativeTimerBindingId;
    claim.expectedBindingRevision = handoff.expectedBindingRevision;
    claim.backendId = handoff.backendId;
    claim.backendGeneration = handoff.backendGeneration;
    claim.backendNativeTimerId = handoff.backendNativeTimerId;
    claim.claimedAt = claimedAt;
    return claim;
}

NativeTimerDeleteDispatchClaimResult claimResult(
    NativeTimerDeleteDispatchClaimStatus status,
    const MutationOperation& operation = {},
    const NativeTimerDeleteDispatchClaim& claim = {})
{
    NativeTimerDeleteDispatchClaimResult result;
    result.status = status;
    result.operation = operation;
    result.claim = claim;
    return result;
}

NativeTimerDeleteDispatchOutcomeResult outcomeResult(
    NativeTimerDeleteDispatchOutcomeStatus status,
    const MutationOperation& operation = {},
    const NativeTimerAbsenceReadbackExpectation* expectation = nullptr)
{
    NativeTimerDeleteDispatchOutcomeResult result;
    result.status = status;
    result.operation = operation;
    if (expectation != nullptr)
    {
        result.expectationPresent = true;
        result.expectation = *expectation;
    }
    return result;
}

bool claimValid(const NativeTimerDeleteDispatchClaim& claim)
{
    return !claim.operationId.empty() && !claim.operationRevision.empty() &&
        !claim.nativeTimerBindingId.empty() &&
        !claim.expectedBindingRevision.empty() && !claim.backendId.empty() &&
        claim.backendGeneration > 0 && !claim.backendNativeTimerId.empty() &&
        claim.claimedAt > 0;
}

bool operationMatchesClaim(
    const MutationOperation& operation,
    const NativeTimerDeleteDispatchClaim& claim)
{
    return operation.operationId == claim.operationId &&
        operation.backendId == claim.backendId &&
        operation.backendGeneration == claim.backendGeneration &&
        operation.resourceType == "NativeTimerBinding" &&
        operation.resourceId == claim.nativeTimerBindingId &&
        operation.expectedRevision == claim.expectedBindingRevision &&
        operation.actionFamily == "timer.delete" &&
        operation.verificationPolicy ==
            MutationOperationVerificationPolicy::readbackRequired;
}

MutationOperationState outcomeState(NativeTimerDeleteExecutorOutcomeCategory category)
{
    switch (category)
    {
        case NativeTimerDeleteExecutorOutcomeCategory::rejectedWithoutEffect:
            return MutationOperationState::failedVerified;
        case NativeTimerDeleteExecutorOutcomeCategory::acceptedUnverified:
            return MutationOperationState::executedUnverified;
        case NativeTimerDeleteExecutorOutcomeCategory::outcomeUnknown:
            return MutationOperationState::outcomeUnknown;
    }
    return MutationOperationState::outcomeUnknown;
}

bool outcomeValid(
    const NativeTimerDeleteDispatchClaim& claim,
    const NativeTimerDeleteExecutorOutcome& outcome)
{
    if (!claimValid(claim) || outcome.completedAt < claim.claimedAt ||
        outcome.evidenceReference.empty() ||
        outcome.evidenceReference.size() > kMaxEvidenceReferenceLength)
        return false;

    switch (outcome.category)
    {
        case NativeTimerDeleteExecutorOutcomeCategory::rejectedWithoutEffect:
            return outcome.dispatchStartedAt == 0;
        case NativeTimerDeleteExecutorOutcomeCategory::acceptedUnverified:
        case NativeTimerDeleteExecutorOutcomeCategory::outcomeUnknown:
            return outcome.dispatchStartedAt >= claim.claimedAt &&
                outcome.dispatchStartedAt <= outcome.completedAt;
    }
    return false;
}

NativeTimerAbsenceReadbackExpectation expectationFor(
    const NativeTimerDeleteDispatchClaim& claim,
    const NativeTimerDeleteExecutorOutcome& outcome)
{
    NativeTimerAbsenceReadbackExpectation expectation;
    expectation.operationId = claim.operationId;
    expectation.operationState =
        outcome.category == NativeTimerDeleteExecutorOutcomeCategory::acceptedUnverified
            ? NativeTimerReadbackOperationState::executedUnverified
            : NativeTimerReadbackOperationState::outcomeUnknown;
    expectation.nativeTimerBindingId = claim.nativeTimerBindingId;
    expectation.expectedBindingRevision = claim.expectedBindingRevision;
    expectation.backendId = claim.backendId;
    expectation.backendGeneration = claim.backendGeneration;
    expectation.backendNativeTimerId = claim.backendNativeTimerId;
    expectation.readbackNotBefore = outcome.dispatchStartedAt;
    return expectation;
}
}

NativeTimerDeleteDispatchService::NativeTimerDeleteDispatchService(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    NativeTimerBindingRepository& bindingRepository)
    : operationRepository_(operationRepository),
      bindingRepository_(bindingRepository)
{
}

NativeTimerDeleteDispatchClaimResult NativeTimerDeleteDispatchService::claim(
    const NativeTimerDeleteDispatchHandoff& handoff,
    std::int64_t claimedAt)
{
    if (handoff.operationId.empty() || handoff.operationRevision.empty() ||
        handoff.nativeTimerBindingId.empty() ||
        handoff.expectedBindingRevision.empty() || handoff.backendId.empty() ||
        handoff.backendGeneration == 0 || handoff.backendNativeTimerId.empty() ||
        handoff.timerAssignmentId.empty() || claimedAt <= 0)
        return claimResult(NativeTimerDeleteDispatchClaimStatus::invalid);

    const auto foundOperation = operationRepository_.findById(handoff.operationId);
    if (foundOperation.status == MutationOperationRepositoryStatus::notFound)
        return claimResult(NativeTimerDeleteDispatchClaimStatus::operationNotFound);
    if (!foundOperation.ok())
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::operationRepositoryError);

    const MutationOperation& operation = foundOperation.operation;
    if (!operationMatchesHandoff(operation, handoff))
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::identityConflict, operation);
    if (operation.state != MutationOperationState::accepted &&
        operation.state != MutationOperationState::dispatching)
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::operationStateConflict,
            operation);
    if (operation.state == MutationOperationState::accepted &&
        operation.operationRevision != handoff.operationRevision)
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::operationRevisionConflict,
            operation);

    const auto foundBinding =
        bindingRepository_.findById(handoff.nativeTimerBindingId);
    if (foundBinding.status == NativeTimerBindingRepositoryStatus::notFound)
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::bindingNotFound, operation);
    if (!foundBinding.ok())
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::bindingRepositoryError,
            operation);

    const NativeTimerBinding& binding = foundBinding.binding;
    if (!bindingMatchesHandoff(binding, handoff))
    {
        if (binding.bindingRevision != handoff.expectedBindingRevision)
            return claimResult(
                NativeTimerDeleteDispatchClaimStatus::bindingRevisionConflict,
                operation);
        if (binding.backendGeneration != handoff.backendGeneration)
            return claimResult(
                NativeTimerDeleteDispatchClaimStatus::generationConflict,
                operation);
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::identityConflict, operation);
    }
    if (!suiteManaged(binding.ownership))
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::ownershipConflict, operation);
    if (binding.missingSince != 0 ||
        binding.driftState != NativeTimerBindingDriftState::none)
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::bindingStateConflict,
            operation);

    if (operation.state == MutationOperationState::dispatching)
    {
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::alreadyClaimed,
            operation,
            claimFrom(operation, handoff, operation.updatedAt));
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
            NativeTimerDeleteDispatchClaimStatus::claimed,
            transitioned.operation,
            claimFrom(transitioned.operation, handoff, claimedAt));
    }
    if (transitioned.status == MutationOperationRepositoryStatus::idempotentReplay)
    {
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::alreadyClaimed,
            transitioned.operation,
            claimFrom(transitioned.operation, handoff, transitioned.operation.updatedAt));
    }
    if (transitioned.status == MutationOperationRepositoryStatus::revisionConflict)
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::operationRevisionConflict,
            transitioned.operation);
    if (transitioned.status == MutationOperationRepositoryStatus::stateConflict)
        return claimResult(
            NativeTimerDeleteDispatchClaimStatus::operationStateConflict,
            transitioned.operation);
    return claimResult(
        NativeTimerDeleteDispatchClaimStatus::operationRepositoryError,
        transitioned.operation);
}

NativeTimerDeleteDispatchOutcomeResult NativeTimerDeleteDispatchService::applyOutcome(
    const NativeTimerDeleteDispatchClaim& claim,
    const NativeTimerDeleteExecutorOutcome& outcome)
{
    if (!outcomeValid(claim, outcome))
        return outcomeResult(NativeTimerDeleteDispatchOutcomeStatus::invalid);

    const auto found = operationRepository_.findById(claim.operationId);
    if (found.status == MutationOperationRepositoryStatus::notFound)
        return outcomeResult(
            NativeTimerDeleteDispatchOutcomeStatus::operationNotFound);
    if (!found.ok())
        return outcomeResult(
            NativeTimerDeleteDispatchOutcomeStatus::operationRepositoryError);

    const MutationOperation& operation = found.operation;
    if (!operationMatchesClaim(operation, claim))
        return outcomeResult(
            NativeTimerDeleteDispatchOutcomeStatus::identityConflict, operation);

    const MutationOperationState target = outcomeState(outcome.category);
    if (operation.state != MutationOperationState::dispatching &&
        operation.state != target)
        return outcomeResult(
            NativeTimerDeleteDispatchOutcomeStatus::operationStateConflict,
            operation);
    if (operation.state == MutationOperationState::dispatching &&
        operation.operationRevision != claim.operationRevision)
        return outcomeResult(
            NativeTimerDeleteDispatchOutcomeStatus::operationRevisionConflict,
            operation);

    NativeTimerAbsenceReadbackExpectation expectation;
    const bool needsReadback =
        outcome.category != NativeTimerDeleteExecutorOutcomeCategory::rejectedWithoutEffect;
    if (needsReadback)
    {
        expectation = expectationFor(claim, outcome);
        if (!nativeTimerAbsenceReadbackExpectationValid(expectation))
            return outcomeResult(NativeTimerDeleteDispatchOutcomeStatus::invalid, operation);
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
            NativeTimerDeleteDispatchOutcomeStatus::applied,
            transitioned.operation,
            needsReadback ? &expectation : nullptr);
    if (transitioned.status == MutationOperationRepositoryStatus::idempotentReplay)
        return outcomeResult(
            NativeTimerDeleteDispatchOutcomeStatus::alreadyApplied,
            transitioned.operation,
            needsReadback ? &expectation : nullptr);
    if (transitioned.status == MutationOperationRepositoryStatus::revisionConflict)
        return outcomeResult(
            NativeTimerDeleteDispatchOutcomeStatus::operationRevisionConflict,
            transitioned.operation);
    if (transitioned.status == MutationOperationRepositoryStatus::stateConflict)
        return outcomeResult(
            NativeTimerDeleteDispatchOutcomeStatus::operationStateConflict,
            transitioned.operation);
    return outcomeResult(
        NativeTimerDeleteDispatchOutcomeStatus::operationRepositoryError,
        transitioned.operation);
}

}
