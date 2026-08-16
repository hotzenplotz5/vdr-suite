#include "NativeTimerModifyReadbackVerificationService.h"

#include "NativeTimerBindingRepository.h"
#include "NativeTimerSpecification.h"

namespace vdrsuite::timers
{
namespace
{
NativeTimerModifyReadbackVerificationResult result(
    NativeTimerModifyReadbackVerificationStatus status,
    const NativeTimerBinding& binding = {})
{
    NativeTimerModifyReadbackVerificationResult value;
    value.status = status;
    value.binding = binding;
    return value;
}

bool owned(NativeTimerBindingOwnership ownership)
{
    return ownership == NativeTimerBindingOwnership::managed
        || ownership == NativeTimerBindingOwnership::adopted;
}

bool alreadyVerified(
    const NativeTimerBinding& binding,
    const NativeTimerModifyReadbackExpectation& expectation,
    const NativeTimerObservation& observation)
{
    return binding.lastVerifiedOperationId == expectation.operationId
        && binding.backendGeneration == observation.backendGeneration
        && binding.observedFingerprint == observation.observedFingerprint
        && binding.lastObservedAt >= expectation.readbackNotBefore
        && binding.missingSince == 0
        && binding.driftState == NativeTimerBindingDriftState::none;
}
}

bool nativeTimerModifyReadbackExpectationValid(
    const NativeTimerModifyReadbackExpectation& expectation)
{
    return !expectation.operationId.empty()
        && expectation.operationId.size() <= 160
        && nativeTimerModifyOperationPayloadValid(expectation.payload)
        && expectation.readbackNotBefore > 0;
}

NativeTimerModifyReadbackVerificationService::
NativeTimerModifyReadbackVerificationService(
    NativeTimerBindingRepository& repository)
    : repository_(repository)
{
}

NativeTimerModifyReadbackVerificationResult
NativeTimerModifyReadbackVerificationService::verify(
    const NativeTimerModifyReadbackExpectation& expectation,
    const NativeTimerObservation& observation)
{
    if (!nativeTimerModifyReadbackExpectationValid(expectation)
        || !nativeTimerObservationValid(observation))
        return result(NativeTimerModifyReadbackVerificationStatus::invalid);

    const auto& payload = expectation.payload;
    const auto found = repository_.findById(payload.nativeTimerBindingId);
    if (found.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(
            NativeTimerModifyReadbackVerificationStatus::bindingNotFound);
    if (!found.ok())
        return result(
            NativeTimerModifyReadbackVerificationStatus::repositoryError);
    const NativeTimerBinding& current = found.binding;

    if (current.timerAssignmentId != payload.timerAssignmentId
        || current.backendId != payload.backendId
        || current.backendNativeTimerId != payload.backendNativeTimerId
        || observation.backendId != payload.backendId
        || observation.backendNativeTimerId != payload.backendNativeTimerId)
        return result(
            NativeTimerModifyReadbackVerificationStatus::identityConflict,
            current);
    if (!owned(current.ownership))
        return result(
            NativeTimerModifyReadbackVerificationStatus::ownershipConflict,
            current);
    if (current.backendGeneration > payload.backendGeneration
        || observation.backendGeneration != payload.backendGeneration)
        return result(
            NativeTimerModifyReadbackVerificationStatus::generationConflict,
            current);
    if (observation.observedAt < expectation.readbackNotBefore
        || observation.observedAt < current.lastObservedAt)
        return result(
            NativeTimerModifyReadbackVerificationStatus::staleObservation,
            current);
    if (!nativeTimerObservationMatchesSpecification(
            payload.expectedSpecification,
            observation.observedState))
        return result(
            NativeTimerModifyReadbackVerificationStatus::reconciliationRequired,
            current);
    if (alreadyVerified(current, expectation, observation))
        return result(
            NativeTimerModifyReadbackVerificationStatus::alreadyVerified,
            current);
    if (current.bindingRevision != payload.expectedBindingRevision)
        return result(
            NativeTimerModifyReadbackVerificationStatus::
                bindingRevisionConflict,
            current);
    if (current.observedFingerprint != payload.expectedCurrentFingerprint)
        return result(
            NativeTimerModifyReadbackVerificationStatus::
                predecessorFingerprintConflict,
            current);

    NativeTimerBinding next = current;
    next.backendGeneration = observation.backendGeneration;
    next.observedState = observation.observedState;
    next.observedFingerprint = observation.observedFingerprint;
    next.lastObservedAt = observation.observedAt;
    next.lastVerifiedOperationId = expectation.operationId;
    next.missingSince = 0;
    next.driftState = NativeTimerBindingDriftState::none;
    const auto updated = repository_.update(next, current.bindingRevision);
    if (updated.status == NativeTimerBindingRepositoryStatus::conflict)
        return result(
            NativeTimerModifyReadbackVerificationStatus::repositoryConflict,
            updated.binding);
    if (!updated.ok())
        return result(
            NativeTimerModifyReadbackVerificationStatus::repositoryError);
    return result(
        NativeTimerModifyReadbackVerificationStatus::verified,
        updated.binding);
}

}
