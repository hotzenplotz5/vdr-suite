#include "NativeTimerModifyOperationCompletionService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerSpecification.h"

#include <string>

namespace vdrsuite::timers
{
namespace
{
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::operations::MutationOperationVerificationPolicy;

NativeTimerModifyOperationCompletionResult result(
    NativeTimerModifyOperationCompletionStatus status,
    const MutationOperation& operation = {},
    const NativeTimerBinding& binding = {})
{
    NativeTimerModifyOperationCompletionResult value;
    value.status = status;
    value.operation = operation;
    value.binding = binding;
    return value;
}

MutationOperationState operationState(
    NativeTimerReadbackOperationState state)
{
    switch (state)
    {
        case NativeTimerReadbackOperationState::executedUnverified:
            return MutationOperationState::executedUnverified;
        case NativeTimerReadbackOperationState::outcomeUnknown:
            return MutationOperationState::outcomeUnknown;
    }
    return MutationOperationState::conflict;
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

std::string completionReference(
    const NativeTimerModifyReadbackExpectation& expectation)
{
    return "native-timer-modify-readback:" +
        expectation.payload.nativeTimerBindingId +
        ":operation:" + expectation.operationId;
}

bool operationIdentityMatches(
    const MutationOperation& operation,
    const NativeTimerModifyReadbackExpectation& expectation)
{
    const auto& payload = expectation.payload;
    return operation.operationId == expectation.operationId &&
        operation.backendId == payload.backendId &&
        operation.backendGeneration == payload.backendGeneration &&
        operation.resourceType == "NativeTimerBinding" &&
        operation.resourceId == payload.nativeTimerBindingId &&
        operation.expectedRevision == payload.expectedBindingRevision &&
        operation.expectedResourceFingerprint ==
            payload.expectedCurrentFingerprint &&
        operation.actionFamily == actionFamily(payload.kind);
}

bool bindingEvidenceMatches(
    const NativeTimerBinding& binding,
    const NativeTimerModifyReadbackExpectation& expectation)
{
    const auto& payload = expectation.payload;
    return binding.nativeTimerBindingId == payload.nativeTimerBindingId &&
        binding.timerAssignmentId == payload.timerAssignmentId &&
        binding.backendId == payload.backendId &&
        binding.backendGeneration == payload.backendGeneration &&
        binding.backendNativeTimerId == payload.backendNativeTimerId &&
        binding.lastVerifiedOperationId == expectation.operationId &&
        binding.lastObservedAt >= expectation.readbackNotBefore &&
        binding.missingSince == 0 &&
        binding.driftState == NativeTimerBindingDriftState::none &&
        suiteManaged(binding.ownership) &&
        nativeTimerObservationMatchesSpecification(
            payload.expectedSpecification,
            binding.observedState);
}
} // namespace

NativeTimerModifyOperationCompletionService::
NativeTimerModifyOperationCompletionService(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    NativeTimerBindingRepository& bindingRepository)
    : operationRepository_(operationRepository),
      bindingRepository_(bindingRepository)
{
}

NativeTimerModifyOperationCompletionResult
NativeTimerModifyOperationCompletionService::complete(
    const NativeTimerModifyReadbackExpectation& expectation,
    std::int64_t completedAt)
{
    if (!nativeTimerModifyReadbackExpectationValid(expectation) ||
        completedAt <= 0)
        return result(
            NativeTimerModifyOperationCompletionStatus::invalid);

    const auto foundOperation =
        operationRepository_.findById(expectation.operationId);
    if (foundOperation.status == MutationOperationRepositoryStatus::notFound)
        return result(
            NativeTimerModifyOperationCompletionStatus::operationNotFound);
    if (!foundOperation.ok())
        return result(
            NativeTimerModifyOperationCompletionStatus::
                operationRepositoryError);

    const MutationOperation& operation = foundOperation.operation;
    if (!operationIdentityMatches(operation, expectation))
        return result(
            NativeTimerModifyOperationCompletionStatus::identityConflict,
            operation);
    if (operation.verificationPolicy !=
        MutationOperationVerificationPolicy::readbackRequired)
        return result(
            NativeTimerModifyOperationCompletionStatus::
                verificationPolicyConflict,
            operation);

    const std::string resultReference = completionReference(expectation);
    if (operation.state == MutationOperationState::succeeded)
    {
        return result(
            operation.resultReference == resultReference
                ? NativeTimerModifyOperationCompletionStatus::alreadyCompleted
                : NativeTimerModifyOperationCompletionStatus::
                    operationStateConflict,
            operation);
    }

    if (operation.state != operationState(expectation.operationState))
        return result(
            NativeTimerModifyOperationCompletionStatus::
                operationStateConflict,
            operation);
    if (completedAt < operation.updatedAt)
        return result(
            NativeTimerModifyOperationCompletionStatus::invalid,
            operation);

    const auto foundBinding =
        bindingRepository_.findById(
            expectation.payload.nativeTimerBindingId);
    if (foundBinding.status ==
        NativeTimerBindingRepositoryStatus::notFound)
        return result(
            NativeTimerModifyOperationCompletionStatus::bindingNotFound,
            operation);
    if (!foundBinding.ok())
        return result(
            NativeTimerModifyOperationCompletionStatus::
                bindingRepositoryError,
            operation);

    const NativeTimerBinding& binding = foundBinding.binding;
    if (!bindingEvidenceMatches(binding, expectation))
        return result(
            NativeTimerModifyOperationCompletionStatus::
                verificationEvidenceMissing,
            operation,
            binding);
    if (completedAt < binding.lastObservedAt)
        return result(
            NativeTimerModifyOperationCompletionStatus::invalid,
            operation,
            binding);

    const auto transitioned = operationRepository_.transition(
        operation.operationId,
        operation.operationRevision,
        operation.state,
        MutationOperationState::succeeded,
        resultReference,
        completedAt);

    switch (transitioned.status)
    {
        case MutationOperationRepositoryStatus::ok:
            return result(
                NativeTimerModifyOperationCompletionStatus::completed,
                transitioned.operation,
                binding);
        case MutationOperationRepositoryStatus::idempotentReplay:
            return result(
                NativeTimerModifyOperationCompletionStatus::alreadyCompleted,
                transitioned.operation,
                binding);
        case MutationOperationRepositoryStatus::notFound:
            return result(
                NativeTimerModifyOperationCompletionStatus::operationNotFound,
                operation,
                binding);
        case MutationOperationRepositoryStatus::revisionConflict:
            return result(
                NativeTimerModifyOperationCompletionStatus::
                    operationRevisionConflict,
                transitioned.operation,
                binding);
        case MutationOperationRepositoryStatus::stateConflict:
            return result(
                NativeTimerModifyOperationCompletionStatus::
                    operationStateConflict,
                transitioned.operation,
                binding);
        case MutationOperationRepositoryStatus::invalid:
        case MutationOperationRepositoryStatus::idempotencyConflict:
        case MutationOperationRepositoryStatus::operationConflict:
        case MutationOperationRepositoryStatus::storageError:
            return result(
                NativeTimerModifyOperationCompletionStatus::
                    operationRepositoryError,
                transitioned.operation,
                binding);
    }
    return result(
        NativeTimerModifyOperationCompletionStatus::operationRepositoryError,
        operation,
        binding);
}

} // namespace vdrsuite::timers
