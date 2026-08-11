#include "NativeTimerDeleteOperationCompletionService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"

#include <string>

namespace vdrsuite::timers
{
namespace
{
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationRepositoryResult;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::operations::MutationOperationVerificationPolicy;

NativeTimerDeleteOperationCompletionResult result(
    NativeTimerDeleteOperationCompletionStatus status,
    const MutationOperation& operation = {},
    const NativeTimerBinding& binding = {})
{
    NativeTimerDeleteOperationCompletionResult value;
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

std::string completionReference(
    const NativeTimerAbsenceReadbackExpectation& expectation)
{
    return "native-timer-delete-readback:" + expectation.nativeTimerBindingId +
        ":operation:" + expectation.operationId;
}

bool operationIdentityMatches(
    const MutationOperation& operation,
    const NativeTimerAbsenceReadbackExpectation& expectation)
{
    return operation.operationId == expectation.operationId &&
        operation.backendId == expectation.backendId &&
        operation.backendGeneration == expectation.backendGeneration &&
        operation.resourceType == "NativeTimerBinding" &&
        operation.resourceId == expectation.nativeTimerBindingId &&
        operation.expectedRevision == expectation.expectedBindingRevision &&
        operation.actionFamily == "timer.delete";
}

bool bindingEvidenceMatches(
    const NativeTimerBinding& binding,
    const NativeTimerAbsenceReadbackExpectation& expectation)
{
    return binding.nativeTimerBindingId == expectation.nativeTimerBindingId &&
        binding.backendId == expectation.backendId &&
        binding.backendGeneration == expectation.backendGeneration &&
        binding.backendNativeTimerId == expectation.backendNativeTimerId &&
        binding.lastVerifiedOperationId == expectation.operationId &&
        binding.missingSince > 0 &&
        binding.lastObservedAt >= expectation.readbackNotBefore &&
        suiteManaged(binding.ownership);
}
}

NativeTimerDeleteOperationCompletionService::
NativeTimerDeleteOperationCompletionService(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    NativeTimerBindingRepository& bindingRepository)
    : operationRepository_(operationRepository),
      bindingRepository_(bindingRepository)
{
}

NativeTimerDeleteOperationCompletionResult
NativeTimerDeleteOperationCompletionService::complete(
    const NativeTimerAbsenceReadbackExpectation& expectation,
    std::int64_t completedAt)
{
    if (!nativeTimerAbsenceReadbackExpectationValid(expectation) || completedAt <= 0)
        return result(NativeTimerDeleteOperationCompletionStatus::invalid);

    const auto foundOperation = operationRepository_.findById(expectation.operationId);
    if (foundOperation.status == MutationOperationRepositoryStatus::notFound)
        return result(NativeTimerDeleteOperationCompletionStatus::operationNotFound);
    if (!foundOperation.ok())
        return result(NativeTimerDeleteOperationCompletionStatus::operationRepositoryError);

    const MutationOperation& operation = foundOperation.operation;
    if (!operationIdentityMatches(operation, expectation))
        return result(
            NativeTimerDeleteOperationCompletionStatus::identityConflict,
            operation);
    if (operation.verificationPolicy != MutationOperationVerificationPolicy::readbackRequired)
        return result(
            NativeTimerDeleteOperationCompletionStatus::verificationPolicyConflict,
            operation);

    const std::string resultReference = completionReference(expectation);
    if (operation.state == MutationOperationState::succeeded)
    {
        return result(
            operation.resultReference == resultReference
                ? NativeTimerDeleteOperationCompletionStatus::alreadyCompleted
                : NativeTimerDeleteOperationCompletionStatus::operationStateConflict,
            operation);
    }

    if (operation.state != operationState(expectation.operationState))
        return result(
            NativeTimerDeleteOperationCompletionStatus::operationStateConflict,
            operation);
    if (completedAt < operation.updatedAt)
        return result(NativeTimerDeleteOperationCompletionStatus::invalid, operation);

    const auto foundBinding = bindingRepository_.findById(expectation.nativeTimerBindingId);
    if (foundBinding.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(
            NativeTimerDeleteOperationCompletionStatus::bindingNotFound,
            operation);
    if (!foundBinding.ok())
        return result(
            NativeTimerDeleteOperationCompletionStatus::bindingRepositoryError,
            operation);

    const NativeTimerBinding& binding = foundBinding.binding;
    if (!bindingEvidenceMatches(binding, expectation))
        return result(
            NativeTimerDeleteOperationCompletionStatus::verificationEvidenceMissing,
            operation,
            binding);
    if (completedAt < binding.lastObservedAt)
        return result(
            NativeTimerDeleteOperationCompletionStatus::invalid,
            operation,
            binding);

    const MutationOperationRepositoryResult transitioned = operationRepository_.transition(
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
                NativeTimerDeleteOperationCompletionStatus::completed,
                transitioned.operation,
                binding);
        case MutationOperationRepositoryStatus::idempotentReplay:
            return result(
                NativeTimerDeleteOperationCompletionStatus::alreadyCompleted,
                transitioned.operation,
                binding);
        case MutationOperationRepositoryStatus::notFound:
            return result(
                NativeTimerDeleteOperationCompletionStatus::operationNotFound,
                operation,
                binding);
        case MutationOperationRepositoryStatus::revisionConflict:
            return result(
                NativeTimerDeleteOperationCompletionStatus::operationRevisionConflict,
                transitioned.operation,
                binding);
        case MutationOperationRepositoryStatus::stateConflict:
            return result(
                NativeTimerDeleteOperationCompletionStatus::operationStateConflict,
                transitioned.operation,
                binding);
        case MutationOperationRepositoryStatus::invalid:
        case MutationOperationRepositoryStatus::idempotencyConflict:
        case MutationOperationRepositoryStatus::operationConflict:
        case MutationOperationRepositoryStatus::storageError:
            return result(
                NativeTimerDeleteOperationCompletionStatus::operationRepositoryError,
                transitioned.operation,
                binding);
    }
    return result(
        NativeTimerDeleteOperationCompletionStatus::operationRepositoryError,
        operation,
        binding);
}

}
