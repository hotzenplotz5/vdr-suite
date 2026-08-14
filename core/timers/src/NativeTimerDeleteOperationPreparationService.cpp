#include "NativeTimerDeleteOperationPreparationService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"

namespace vdrsuite::timers
{
namespace
{
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::operations::MutationOperationVerificationPolicy;

NativeTimerDeleteOperationPreparationResult result(
    NativeTimerDeleteOperationPreparationStatus status,
    const MutationOperation& operation = {},
    const NativeTimerBinding& binding = {},
    const NativeTimerDeleteDispatchHandoff& handoff = {})
{
    NativeTimerDeleteOperationPreparationResult value;
    value.status = status;
    value.operation = operation;
    value.binding = binding;
    value.handoff = handoff;
    return value;
}

bool suiteManaged(NativeTimerBindingOwnership ownership)
{
    return ownership == NativeTimerBindingOwnership::managed ||
        ownership == NativeTimerBindingOwnership::adopted;
}

NativeTimerDeleteDispatchHandoff handoffFor(
    const MutationOperation& operation,
    const NativeTimerBinding& binding)
{
    NativeTimerDeleteDispatchHandoff handoff;
    handoff.operationId = operation.operationId;
    handoff.operationRevision = operation.operationRevision;
    handoff.nativeTimerBindingId = binding.nativeTimerBindingId;
    handoff.expectedBindingRevision = operation.expectedRevision;
    handoff.timerAssignmentId = binding.timerAssignmentId;
    handoff.backendId = binding.backendId;
    handoff.backendGeneration = operation.backendGeneration;
    handoff.backendNativeTimerId = binding.backendNativeTimerId;
    return handoff;
}

MutationOperation operationFor(
    const NativeTimerDeleteOperationPreparationRequest& request,
    const NativeTimerBinding& binding)
{
    MutationOperation operation;
    operation.operationId = request.operationId;
    operation.idempotencyKey = request.idempotencyKey;
    operation.actorId = request.actorId;
    operation.backendId = binding.backendId;
    operation.backendGeneration = request.expectedBackendGeneration;
    operation.resourceType = "NativeTimerBinding";
    operation.resourceId = binding.nativeTimerBindingId;
    operation.expectedRevision = request.expectedBindingRevision;
    operation.actionFamily = "timer.delete";
    operation.requestFingerprint = request.requestFingerprint;
    operation.requestedAt = request.requestedAt;
    operation.deadline = request.deadline;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    operation.state = MutationOperationState::accepted;
    operation.updatedAt = request.requestedAt;
    return operation;
}
}

NativeTimerDeleteOperationPreparationService::
NativeTimerDeleteOperationPreparationService(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    NativeTimerBindingRepository& bindingRepository)
    : operationRepository_(operationRepository),
      bindingRepository_(bindingRepository)
{
}

NativeTimerDeleteOperationPreparationResult
NativeTimerDeleteOperationPreparationService::prepare(
    const NativeTimerDeleteOperationPreparationRequest& request)
{
    if (request.operationId.empty() || request.idempotencyKey.empty() ||
        request.actorId.empty() || request.requestFingerprint.empty() ||
        request.nativeTimerBindingId.empty() ||
        request.expectedBindingRevision.empty() ||
        request.expectedBackendGeneration == 0 || request.requestedAt <= 0 ||
        (request.deadline != 0 && request.deadline < request.requestedAt))
    {
        return result(NativeTimerDeleteOperationPreparationStatus::invalid);
    }

    const auto foundBinding =
        bindingRepository_.findById(request.nativeTimerBindingId);
    if (foundBinding.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(NativeTimerDeleteOperationPreparationStatus::bindingNotFound);
    if (!foundBinding.ok())
        return result(
            NativeTimerDeleteOperationPreparationStatus::bindingRepositoryError);

    const NativeTimerBinding& binding = foundBinding.binding;
    if (!suiteManaged(binding.ownership))
        return result(
            NativeTimerDeleteOperationPreparationStatus::ownershipConflict,
            {}, binding);
    if (!nativeTimerBindingRevisionMatches(
            request.expectedBindingRevision, binding.bindingRevision))
    {
        return result(
            NativeTimerDeleteOperationPreparationStatus::bindingRevisionConflict,
            {}, binding);
    }
    if (binding.backendGeneration != request.expectedBackendGeneration)
        return result(
            NativeTimerDeleteOperationPreparationStatus::generationConflict,
            {}, binding);
    if (binding.missingSince != 0)
        return result(
            NativeTimerDeleteOperationPreparationStatus::bindingMissing,
            {}, binding);
    if (binding.driftState != NativeTimerBindingDriftState::none)
        return result(
            NativeTimerDeleteOperationPreparationStatus::driftConflict,
            {}, binding);

    const MutationOperation candidate = operationFor(request, binding);
    const auto reserved = operationRepository_.reserve(candidate);
    switch (reserved.status)
    {
        case MutationOperationRepositoryStatus::ok:
        {
            const auto handoff = handoffFor(reserved.operation, binding);
            return result(
                NativeTimerDeleteOperationPreparationStatus::prepared,
                reserved.operation,
                binding,
                handoff);
        }
        case MutationOperationRepositoryStatus::idempotentReplay:
        {
            if (reserved.operation.state != MutationOperationState::accepted)
            {
                return result(
                    NativeTimerDeleteOperationPreparationStatus::operationStateConflict,
                    reserved.operation,
                    binding);
            }
            const auto handoff = handoffFor(reserved.operation, binding);
            return result(
                NativeTimerDeleteOperationPreparationStatus::alreadyPrepared,
                reserved.operation,
                binding,
                handoff);
        }
        case MutationOperationRepositoryStatus::idempotencyConflict:
            return result(
                NativeTimerDeleteOperationPreparationStatus::idempotencyConflict,
                reserved.operation,
                binding);
        case MutationOperationRepositoryStatus::operationConflict:
            return result(
                NativeTimerDeleteOperationPreparationStatus::operationConflict,
                reserved.operation,
                binding);
        case MutationOperationRepositoryStatus::invalid:
            return result(
                NativeTimerDeleteOperationPreparationStatus::invalid,
                reserved.operation,
                binding);
        case MutationOperationRepositoryStatus::notFound:
        case MutationOperationRepositoryStatus::revisionConflict:
        case MutationOperationRepositoryStatus::stateConflict:
        case MutationOperationRepositoryStatus::storageError:
            return result(
                NativeTimerDeleteOperationPreparationStatus::operationRepositoryError,
                reserved.operation,
                binding);
    }

    return result(
        NativeTimerDeleteOperationPreparationStatus::operationRepositoryError,
        {}, binding);
}

}
