#include "NativeTimerBindingAbsenceApplicationService.h"

#include "NativeTimerBindingRepository.h"

#include <cstddef>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

NativeTimerBindingAbsenceApplicationResult result(
    NativeTimerBindingAbsenceApplicationStatus status,
    const NativeTimerBinding& binding = {})
{
    NativeTimerBindingAbsenceApplicationResult value;
    value.status = status;
    value.binding = binding;
    return value;
}

NativeTimerBindingDriftState firstMissingDrift(
    NativeTimerBindingDriftState current)
{
    return current == NativeTimerBindingDriftState::expectedTransition
        ? NativeTimerBindingDriftState::expectedTransition
        : NativeTimerBindingDriftState::ambiguous;
}
}

NativeTimerBindingAbsenceApplicationService::
NativeTimerBindingAbsenceApplicationService(
    NativeTimerBindingRepository& repository)
    : repository_(repository)
{
}

NativeTimerBindingAbsenceApplicationResult
NativeTimerBindingAbsenceApplicationService::apply(
    const std::string& nativeTimerBindingId,
    const NativeTimerInventoryEvidence& evidence)
{
    if (!validIdentity(nativeTimerBindingId)
        || !nativeTimerInventoryEvidenceValid(evidence))
    {
        return result(NativeTimerBindingAbsenceApplicationStatus::invalid);
    }

    const auto found = repository_.findById(nativeTimerBindingId);
    if (found.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(NativeTimerBindingAbsenceApplicationStatus::bindingNotFound);
    if (!found.ok())
        return result(NativeTimerBindingAbsenceApplicationStatus::repositoryError);

    const NativeTimerBinding& current = found.binding;
    if (evidence.backendId != current.backendId)
    {
        return result(
            NativeTimerBindingAbsenceApplicationStatus::backendConflict,
            current);
    }
    if (evidence.backendGeneration < current.backendGeneration)
    {
        return result(
            NativeTimerBindingAbsenceApplicationStatus::staleGeneration,
            current);
    }
    if (evidence.observedAt < current.lastObservedAt)
    {
        return result(
            NativeTimerBindingAbsenceApplicationStatus::staleEvidence,
            current);
    }

    NativeTimerAbsenceAssessmentRequest request;
    request.backendId = current.backendId;
    request.backendGeneration = evidence.backendGeneration;
    request.backendNativeTimerId = current.backendNativeTimerId;
    request.notBefore = current.lastObservedAt;

    const auto assessment = assessNativeTimerAbsence(evidence, request);
    if (assessment == NativeTimerAbsenceAssessmentStatus::present)
    {
        return result(
            current.missingSince == 0
                ? NativeTimerBindingAbsenceApplicationStatus::present
                : NativeTimerBindingAbsenceApplicationStatus::reconciliationRequired,
            current);
    }
    if (assessment != NativeTimerAbsenceAssessmentStatus::absent)
        return result(NativeTimerBindingAbsenceApplicationStatus::invalid, current);

    if (current.missingSince != 0
        && evidence.backendGeneration == current.backendGeneration
        && evidence.observedAt == current.lastObservedAt)
    {
        return result(
            NativeTimerBindingAbsenceApplicationStatus::alreadyCurrent,
            current);
    }

    NativeTimerBinding next = current;
    next.backendGeneration = evidence.backendGeneration;
    next.lastObservedAt = evidence.observedAt;

    const bool firstMissing = current.missingSince == 0;
    if (firstMissing)
    {
        next.missingSince = evidence.observedAt;
        next.driftState = firstMissingDrift(current.driftState);
    }

    const auto updated = repository_.update(next, current.bindingRevision);
    if (updated.status == NativeTimerBindingRepositoryStatus::conflict)
    {
        return result(
            NativeTimerBindingAbsenceApplicationStatus::repositoryConflict,
            updated.binding);
    }
    if (!updated.ok())
        return result(NativeTimerBindingAbsenceApplicationStatus::repositoryError);

    return result(
        firstMissing
            ? NativeTimerBindingAbsenceApplicationStatus::missingRecorded
            : NativeTimerBindingAbsenceApplicationStatus::missingRefreshed,
        updated.binding);
}

}
