#include "NativeTimerPresentReadbackVerificationService.h"

#include "NativeTimerBindingRepository.h"

namespace vdrsuite::timers
{
namespace
{
NativeTimerPresentReadbackVerificationResult result(
    NativeTimerPresentReadbackVerificationStatus status,
    const NativeTimerBinding& binding = {})
{
    NativeTimerPresentReadbackVerificationResult value;
    value.status = status;
    value.binding = binding;
    return value;
}

bool ownedForSuiteMutation(NativeTimerBindingOwnership ownership)
{
    return ownership == NativeTimerBindingOwnership::managed
        || ownership == NativeTimerBindingOwnership::adopted;
}

bool expectationMatchesBindingIdentity(
    const NativeTimerReadbackExpectation& expectation,
    const NativeTimerBinding& binding)
{
    return expectation.nativeTimerBindingId == binding.nativeTimerBindingId
        && expectation.backendId == binding.backendId
        && expectation.backendNativeTimerId == binding.backendNativeTimerId;
}

bool observationMatchesExpectationIdentity(
    const NativeTimerObservation& observation,
    const NativeTimerReadbackExpectation& expectation)
{
    return observation.backendId == expectation.backendId
        && observation.backendNativeTimerId == expectation.backendNativeTimerId;
}

bool alreadyVerified(
    const NativeTimerBinding& current,
    const NativeTimerReadbackExpectation& expectation)
{
    return current.lastVerifiedOperationId == expectation.operationId
        && current.backendGeneration == expectation.backendGeneration
        && current.observedFingerprint == expectation.expectedFingerprint
        && current.missingSince == 0
        && current.driftState == NativeTimerBindingDriftState::none
        && current.lastObservedAt >= expectation.readbackNotBefore;
}
}

NativeTimerPresentReadbackVerificationService::
NativeTimerPresentReadbackVerificationService(
    NativeTimerBindingRepository& repository)
    : repository_(repository)
{
}

NativeTimerPresentReadbackVerificationResult
NativeTimerPresentReadbackVerificationService::verify(
    const NativeTimerReadbackExpectation& expectation,
    const NativeTimerObservation& observation)
{
    if (!nativeTimerReadbackExpectationValid(expectation)
        || !nativeTimerObservationValid(observation))
    {
        return result(NativeTimerPresentReadbackVerificationStatus::invalid);
    }

    const auto found = repository_.findById(expectation.nativeTimerBindingId);
    if (found.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(
            NativeTimerPresentReadbackVerificationStatus::bindingNotFound);
    if (!found.ok())
        return result(NativeTimerPresentReadbackVerificationStatus::repositoryError);

    const NativeTimerBinding& current = found.binding;

    if (!expectationMatchesBindingIdentity(expectation, current)
        || !observationMatchesExpectationIdentity(observation, expectation))
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::identityConflict,
            current);
    }

    if (!ownedForSuiteMutation(current.ownership))
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::ownershipConflict,
            current);
    }

    if (observation.backendGeneration != expectation.backendGeneration
        || current.backendGeneration > expectation.backendGeneration)
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::generationConflict,
            current);
    }

    if (observation.observedAt < expectation.readbackNotBefore)
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::staleObservation,
            current);
    }

    if (observation.observedFingerprint != expectation.expectedFingerprint)
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::reconciliationRequired,
            current);
    }

    if (alreadyVerified(current, expectation))
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::alreadyVerified,
            current);
    }

    if (!nativeTimerBindingRevisionMatches(
            expectation.expectedBindingRevision,
            current.bindingRevision))
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::bindingRevisionConflict,
            current);
    }

    if (observation.observedAt < current.lastObservedAt)
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::staleObservation,
            current);
    }

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
    {
        return result(
            NativeTimerPresentReadbackVerificationStatus::repositoryConflict,
            updated.binding);
    }
    if (!updated.ok())
        return result(NativeTimerPresentReadbackVerificationStatus::repositoryError);

    return result(
        NativeTimerPresentReadbackVerificationStatus::verified,
        updated.binding);
}

}
