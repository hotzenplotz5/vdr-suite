#include "NativeTimerAbsenceReadbackVerificationService.h"

#include "NativeTimerBindingRepository.h"

namespace vdrsuite::timers
{
namespace
{
NativeTimerAbsenceReadbackVerificationResult result(
    NativeTimerAbsenceReadbackVerificationStatus status,
    const NativeTimerBinding& binding = {})
{
    NativeTimerAbsenceReadbackVerificationResult value;
    value.status = status;
    value.binding = binding;
    return value;
}

bool suiteManaged(NativeTimerBindingOwnership ownership)
{
    return ownership == NativeTimerBindingOwnership::managed
        || ownership == NativeTimerBindingOwnership::adopted;
}

bool identityMatches(
    const NativeTimerBinding& current,
    const NativeTimerAbsenceReadbackExpectation& expectation)
{
    return current.nativeTimerBindingId == expectation.nativeTimerBindingId
        && current.backendId == expectation.backendId
        && current.backendNativeTimerId == expectation.backendNativeTimerId;
}

bool alreadyVerified(
    const NativeTimerBinding& current,
    const NativeTimerAbsenceReadbackExpectation& expectation)
{
    return current.lastVerifiedOperationId == expectation.operationId
        && current.backendGeneration == expectation.backendGeneration
        && current.missingSince != 0
        && current.lastObservedAt >= expectation.readbackNotBefore;
}
}

NativeTimerAbsenceReadbackVerificationService::
NativeTimerAbsenceReadbackVerificationService(
    NativeTimerBindingRepository& repository)
    : repository_(repository)
{
}

NativeTimerAbsenceReadbackVerificationResult
NativeTimerAbsenceReadbackVerificationService::verify(
    const NativeTimerAbsenceReadbackExpectation& expectation,
    const NativeTimerInventoryEvidence& evidence)
{
    if (!nativeTimerAbsenceReadbackExpectationValid(expectation)
        || !nativeTimerInventoryEvidenceValid(evidence))
    {
        return result(NativeTimerAbsenceReadbackVerificationStatus::invalid);
    }

    const auto found = repository_.findById(expectation.nativeTimerBindingId);
    if (found.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(NativeTimerAbsenceReadbackVerificationStatus::bindingNotFound);
    if (!found.ok())
        return result(NativeTimerAbsenceReadbackVerificationStatus::repositoryError);

    const NativeTimerBinding& current = found.binding;
    if (!identityMatches(current, expectation))
        return result(
            NativeTimerAbsenceReadbackVerificationStatus::identityConflict,
            current);
    if (!suiteManaged(current.ownership))
        return result(
            NativeTimerAbsenceReadbackVerificationStatus::ownershipConflict,
            current);
    if (current.backendGeneration > expectation.backendGeneration)
        return result(
            NativeTimerAbsenceReadbackVerificationStatus::generationConflict,
            current);

    NativeTimerAbsenceAssessmentRequest request;
    request.backendId = expectation.backendId;
    request.backendGeneration = expectation.backendGeneration;
    request.backendNativeTimerId = expectation.backendNativeTimerId;
    request.notBefore = expectation.readbackNotBefore;

    const auto assessment = assessNativeTimerAbsence(evidence, request);
    switch (assessment)
    {
        case NativeTimerAbsenceAssessmentStatus::backendConflict:
            return result(
                NativeTimerAbsenceReadbackVerificationStatus::identityConflict,
                current);
        case NativeTimerAbsenceAssessmentStatus::generationConflict:
            return result(
                NativeTimerAbsenceReadbackVerificationStatus::generationConflict,
                current);
        case NativeTimerAbsenceAssessmentStatus::staleEvidence:
            return result(
                NativeTimerAbsenceReadbackVerificationStatus::staleEvidence,
                current);
        case NativeTimerAbsenceAssessmentStatus::present:
            return result(
                NativeTimerAbsenceReadbackVerificationStatus::reconciliationRequired,
                current);
        case NativeTimerAbsenceAssessmentStatus::invalid:
            return result(NativeTimerAbsenceReadbackVerificationStatus::invalid, current);
        case NativeTimerAbsenceAssessmentStatus::absent:
            break;
    }

    // Current incoming evidence must still prove the expected absence before a
    // previous verification can be replayed idempotently.
    if (alreadyVerified(current, expectation))
        return result(
            NativeTimerAbsenceReadbackVerificationStatus::alreadyVerified,
            current);

    if (!nativeTimerBindingRevisionMatches(
            expectation.expectedBindingRevision,
            current.bindingRevision))
    {
        return result(
            NativeTimerAbsenceReadbackVerificationStatus::bindingRevisionConflict,
            current);
    }
    if (evidence.observedAt < current.lastObservedAt)
        return result(
            NativeTimerAbsenceReadbackVerificationStatus::staleEvidence,
            current);

    NativeTimerBinding next = current;
    next.backendGeneration = evidence.backendGeneration;
    next.lastObservedAt = evidence.observedAt;
    next.lastVerifiedOperationId = expectation.operationId;

    // Verification proves the Suite delete postcondition, not historical cause.
    // Preserve pre-existing missing evidence/classification; only a newly proven
    // absence is classified as the expected Suite transition.
    if (current.missingSince == 0)
    {
        next.missingSince = evidence.observedAt;
        next.driftState = NativeTimerBindingDriftState::expectedTransition;
    }

    const auto updated = repository_.update(next, current.bindingRevision);
    if (updated.status == NativeTimerBindingRepositoryStatus::conflict)
        return result(
            NativeTimerAbsenceReadbackVerificationStatus::repositoryConflict,
            updated.binding);
    if (!updated.ok())
        return result(NativeTimerAbsenceReadbackVerificationStatus::repositoryError);

    return result(
        NativeTimerAbsenceReadbackVerificationStatus::verified,
        updated.binding);
}

}
