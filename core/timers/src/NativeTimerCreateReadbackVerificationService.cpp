#include "NativeTimerCreateReadbackVerificationService.h"

#include "NativeTimerBindingRepository.h"
#include "NativeTimerSpecification.h"

namespace vdrsuite::timers
{
namespace
{
NativeTimerCreateReadbackVerificationResult result(
    NativeTimerCreateReadbackVerificationStatus status,
    const NativeTimerBinding& binding = {})
{
    NativeTimerCreateReadbackVerificationResult value;
    value.status = status;
    value.binding = binding;
    return value;
}

bool correlationMatches(
    const NativeTimerCreateReadbackCandidate& candidate,
    const NativeTimerCreateReadbackExpectation& expectation)
{
    return candidate.timerAssignmentId == expectation.timerAssignmentId
        && candidate.nativeTimerBindingId == expectation.nativeTimerBindingId;
}

bool alreadyVerified(
    const NativeTimerBinding& binding,
    const NativeTimerCreateReadbackExpectation& expectation,
    const NativeTimerCreateReadbackCandidate& candidate)
{
    const NativeTimerObservation& observation = candidate.observation;
    return binding.nativeTimerBindingId == expectation.nativeTimerBindingId
        && binding.timerAssignmentId == expectation.timerAssignmentId
        && binding.backendId == expectation.backendId
        && binding.backendGeneration == expectation.backendGeneration
        && binding.backendNativeTimerId == observation.backendNativeTimerId
        && binding.ownership == NativeTimerBindingOwnership::managed
        && binding.observedFingerprint == observation.observedFingerprint
        && binding.lastObservedAt >= expectation.readbackNotBefore
        && binding.lastVerifiedOperationId == expectation.operationId
        && binding.missingSince == 0
        && binding.driftState == NativeTimerBindingDriftState::none;
}
}

NativeTimerCreateReadbackVerificationService::
NativeTimerCreateReadbackVerificationService(
    NativeTimerBindingRepository& repository)
    : repository_(repository)
{
}

NativeTimerCreateReadbackVerificationResult
NativeTimerCreateReadbackVerificationService::verify(
    const NativeTimerCreateReadbackExpectation& expectation,
    const NativeTimerCreateReadbackEvidence& evidence)
{
    if (!nativeTimerCreateReadbackExpectationValid(expectation)
        || !nativeTimerCreateReadbackEvidenceValid(evidence))
    {
        return result(NativeTimerCreateReadbackVerificationStatus::invalid);
    }

    if (evidence.backendId != expectation.backendId)
        return result(NativeTimerCreateReadbackVerificationStatus::backendConflict);
    if (evidence.backendGeneration != expectation.backendGeneration)
        return result(NativeTimerCreateReadbackVerificationStatus::generationConflict);
    if (evidence.observedAt < expectation.readbackNotBefore)
        return result(NativeTimerCreateReadbackVerificationStatus::staleEvidence);

    const NativeTimerCreateReadbackCandidate* matched = nullptr;
    for (const auto& candidate : evidence.candidates)
    {
        if (!correlationMatches(candidate, expectation)) continue;
        if (matched != nullptr)
            return result(
                NativeTimerCreateReadbackVerificationStatus::correlationAmbiguous);
        matched = &candidate;
    }

    if (matched == nullptr)
        return result(NativeTimerCreateReadbackVerificationStatus::correlationNotFound);

    if (!nativeTimerObservationMatchesSpecification(
            expectation.expectedSpecification,
            matched->observation.observedState))
    {
        return result(
            NativeTimerCreateReadbackVerificationStatus::reconciliationRequired);
    }

    const auto existing = repository_.findById(expectation.nativeTimerBindingId);
    if (existing.ok())
    {
        if (alreadyVerified(existing.binding, expectation, *matched))
        {
            return result(
                NativeTimerCreateReadbackVerificationStatus::alreadyVerified,
                existing.binding);
        }
        return result(
            NativeTimerCreateReadbackVerificationStatus::bindingConflict,
            existing.binding);
    }
    if (existing.status != NativeTimerBindingRepositoryStatus::notFound)
        return result(NativeTimerCreateReadbackVerificationStatus::repositoryError);

    NativeTimerBinding binding;
    binding.nativeTimerBindingId = expectation.nativeTimerBindingId;
    binding.backendId = expectation.backendId;
    binding.backendGeneration = expectation.backendGeneration;
    binding.backendNativeTimerId = matched->observation.backendNativeTimerId;
    binding.timerAssignmentId = expectation.timerAssignmentId;
    binding.ownership = NativeTimerBindingOwnership::managed;
    binding.observedState = matched->observation.observedState;
    binding.observedFingerprint = matched->observation.observedFingerprint;
    binding.lastObservedAt = matched->observation.observedAt;
    binding.lastVerifiedOperationId = expectation.operationId;
    binding.missingSince = 0;
    binding.driftState = NativeTimerBindingDriftState::none;

    const auto created = repository_.create(binding);
    if (created.ok())
    {
        return result(
            NativeTimerCreateReadbackVerificationStatus::verified,
            created.binding);
    }

    switch (created.status)
    {
        case NativeTimerBindingRepositoryStatus::alreadyExists:
            if (alreadyVerified(created.binding, expectation, *matched))
            {
                return result(
                    NativeTimerCreateReadbackVerificationStatus::alreadyVerified,
                    created.binding);
            }
            return result(
                NativeTimerCreateReadbackVerificationStatus::bindingConflict,
                created.binding);
        case NativeTimerBindingRepositoryStatus::nativeIdentityConflict:
            return result(
                NativeTimerCreateReadbackVerificationStatus::nativeIdentityConflict,
                created.binding);
        case NativeTimerBindingRepositoryStatus::assignmentBindingConflict:
            return result(
                NativeTimerCreateReadbackVerificationStatus::assignmentBindingConflict,
                created.binding);
        default:
            return result(NativeTimerCreateReadbackVerificationStatus::repositoryError);
    }
}

}
