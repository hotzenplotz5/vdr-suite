#include "NativeTimerCreateOperationCompletionService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerSpecification.h"
#include "TimerAssignmentRepository.h"

#include <string>

namespace vdrsuite::timers
{
namespace
{
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::operations::MutationOperationVerificationPolicy;

NativeTimerCreateOperationCompletionResult result(
    NativeTimerCreateOperationCompletionStatus status,
    const MutationOperation& operation = {},
    const TimerAssignment& assignment = {},
    const NativeTimerBinding& binding = {})
{
    NativeTimerCreateOperationCompletionResult value;
    value.status = status;
    value.operation = operation;
    value.assignment = assignment;
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

std::string completionReference(
    const NativeTimerCreateReadbackExpectation& expectation)
{
    return "native-timer-create-readback:" +
        expectation.nativeTimerBindingId +
        ":operation:" + expectation.operationId;
}

bool operationIdentityMatches(
    const MutationOperation& operation,
    const NativeTimerCreateReadbackExpectation& expectation)
{
    return operation.operationId == expectation.operationId &&
        operation.backendId == expectation.backendId &&
        operation.backendGeneration == expectation.backendGeneration &&
        operation.resourceType == "TimerAssignment" &&
        operation.resourceId == expectation.timerAssignmentId &&
        operation.expectedResourceFingerprint ==
            expectation.expectedSpecificationFingerprint &&
        operation.actionFamily == "timer.create";
}

bool assignmentEvidenceMatches(
    const TimerAssignment& assignment,
    const NativeTimerCreateReadbackExpectation& expectation)
{
    return assignment.timerAssignmentId == expectation.timerAssignmentId &&
        assignment.backendId == expectation.backendId &&
        assignment.backendGeneration == expectation.backendGeneration &&
        assignment.nativeTimerBindingId ==
            expectation.nativeTimerBindingId &&
        assignment.state == TimerAssignmentState::bound;
}

bool bindingEvidenceMatches(
    const NativeTimerBinding& binding,
    const NativeTimerCreateReadbackExpectation& expectation)
{
    return binding.nativeTimerBindingId ==
            expectation.nativeTimerBindingId &&
        binding.timerAssignmentId == expectation.timerAssignmentId &&
        binding.backendId == expectation.backendId &&
        binding.backendGeneration == expectation.backendGeneration &&
        !binding.backendNativeTimerId.empty() &&
        binding.ownership == NativeTimerBindingOwnership::managed &&
        binding.lastVerifiedOperationId == expectation.operationId &&
        binding.lastObservedAt >= expectation.readbackNotBefore &&
        binding.missingSince == 0 &&
        binding.driftState == NativeTimerBindingDriftState::none &&
        nativeTimerObservationMatchesSpecification(
            expectation.expectedSpecification,
            binding.observedState);
}
} // namespace

NativeTimerCreateOperationCompletionService::
NativeTimerCreateOperationCompletionService(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    TimerAssignmentRepository& assignmentRepository,
    NativeTimerBindingRepository& bindingRepository)
    : operationRepository_(operationRepository),
      assignmentRepository_(assignmentRepository),
      bindingRepository_(bindingRepository)
{
}

NativeTimerCreateOperationCompletionResult
NativeTimerCreateOperationCompletionService::complete(
    const NativeTimerCreateReadbackExpectation& expectation,
    std::int64_t completedAt)
{
    if (!nativeTimerCreateReadbackExpectationValid(expectation) ||
        completedAt <= 0)
        return result(
            NativeTimerCreateOperationCompletionStatus::invalid);

    const auto foundOperation =
        operationRepository_.findById(expectation.operationId);
    if (foundOperation.status == MutationOperationRepositoryStatus::notFound)
        return result(
            NativeTimerCreateOperationCompletionStatus::operationNotFound);
    if (!foundOperation.ok())
        return result(
            NativeTimerCreateOperationCompletionStatus::
                operationRepositoryError);

    const MutationOperation& operation = foundOperation.operation;
    if (!operationIdentityMatches(operation, expectation))
        return result(
            NativeTimerCreateOperationCompletionStatus::identityConflict,
            operation);
    if (operation.verificationPolicy !=
        MutationOperationVerificationPolicy::readbackRequired)
        return result(
            NativeTimerCreateOperationCompletionStatus::
                verificationPolicyConflict,
            operation);

    const std::string resultReference = completionReference(expectation);
    if (operation.state == MutationOperationState::succeeded)
    {
        return result(
            operation.resultReference == resultReference
                ? NativeTimerCreateOperationCompletionStatus::alreadyCompleted
                : NativeTimerCreateOperationCompletionStatus::
                    operationStateConflict,
            operation);
    }
    if (operation.state != operationState(expectation.operationState))
        return result(
            NativeTimerCreateOperationCompletionStatus::
                operationStateConflict,
            operation);
    if (completedAt < operation.updatedAt)
        return result(
            NativeTimerCreateOperationCompletionStatus::invalid,
            operation);

    const auto foundAssignment =
        assignmentRepository_.findById(expectation.timerAssignmentId);
    if (foundAssignment.status == TimerAssignmentRepositoryStatus::notFound)
        return result(
            NativeTimerCreateOperationCompletionStatus::assignmentNotFound,
            operation);
    if (!foundAssignment.ok())
        return result(
            NativeTimerCreateOperationCompletionStatus::
                assignmentRepositoryError,
            operation);

    const TimerAssignment& assignment = foundAssignment.assignment;
    if (!assignmentEvidenceMatches(assignment, expectation))
        return result(
            NativeTimerCreateOperationCompletionStatus::
                verificationEvidenceMissing,
            operation,
            assignment);

    const auto foundBinding =
        bindingRepository_.findById(expectation.nativeTimerBindingId);
    if (foundBinding.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(
            NativeTimerCreateOperationCompletionStatus::bindingNotFound,
            operation,
            assignment);
    if (!foundBinding.ok())
        return result(
            NativeTimerCreateOperationCompletionStatus::
                bindingRepositoryError,
            operation,
            assignment);

    const NativeTimerBinding& binding = foundBinding.binding;
    if (!bindingEvidenceMatches(binding, expectation))
        return result(
            NativeTimerCreateOperationCompletionStatus::
                verificationEvidenceMissing,
            operation,
            assignment,
            binding);
    if (completedAt < binding.lastObservedAt ||
        completedAt < assignment.updatedAt)
        return result(
            NativeTimerCreateOperationCompletionStatus::invalid,
            operation,
            assignment,
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
                NativeTimerCreateOperationCompletionStatus::completed,
                transitioned.operation,
                assignment,
                binding);
        case MutationOperationRepositoryStatus::idempotentReplay:
            return result(
                NativeTimerCreateOperationCompletionStatus::alreadyCompleted,
                transitioned.operation,
                assignment,
                binding);
        case MutationOperationRepositoryStatus::notFound:
            return result(
                NativeTimerCreateOperationCompletionStatus::operationNotFound,
                operation,
                assignment,
                binding);
        case MutationOperationRepositoryStatus::revisionConflict:
            return result(
                NativeTimerCreateOperationCompletionStatus::
                    operationRevisionConflict,
                transitioned.operation,
                assignment,
                binding);
        case MutationOperationRepositoryStatus::stateConflict:
            return result(
                NativeTimerCreateOperationCompletionStatus::
                    operationStateConflict,
                transitioned.operation,
                assignment,
                binding);
        case MutationOperationRepositoryStatus::invalid:
        case MutationOperationRepositoryStatus::idempotencyConflict:
        case MutationOperationRepositoryStatus::operationConflict:
        case MutationOperationRepositoryStatus::storageError:
            return result(
                NativeTimerCreateOperationCompletionStatus::
                    operationRepositoryError,
                transitioned.operation,
                assignment,
                binding);
    }
    return result(
        NativeTimerCreateOperationCompletionStatus::operationRepositoryError,
        operation,
        assignment,
        binding);
}

} // namespace vdrsuite::timers
