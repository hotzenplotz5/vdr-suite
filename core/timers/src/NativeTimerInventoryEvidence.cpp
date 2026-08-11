#include "NativeTimerInventoryEvidence.h"

#include <algorithm>
#include <cstddef>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxInventorySize = 4096;

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool complete(NativeTimerInventoryCompleteness value)
{
    return value == NativeTimerInventoryCompleteness::complete;
}
}

bool nativeTimerInventoryEvidenceValid(
    const NativeTimerInventoryEvidence& evidence)
{
    if (!validIdentity(evidence.backendId)
        || evidence.backendGeneration == 0
        || evidence.observedAt <= 0
        || !complete(evidence.completeness)
        || evidence.backendNativeTimerIds.size() > kMaxInventorySize)
    {
        return false;
    }

    std::string previous;
    bool first = true;
    for (const auto& id : evidence.backendNativeTimerIds)
    {
        if (!validIdentity(id)) return false;
        if (!first && !(previous < id)) return false;
        previous = id;
        first = false;
    }
    return true;
}

bool nativeTimerAbsenceAssessmentRequestValid(
    const NativeTimerAbsenceAssessmentRequest& request)
{
    return validIdentity(request.backendId)
        && request.backendGeneration != 0
        && validIdentity(request.backendNativeTimerId)
        && request.notBefore > 0;
}

NativeTimerAbsenceAssessmentStatus assessNativeTimerAbsence(
    const NativeTimerInventoryEvidence& evidence,
    const NativeTimerAbsenceAssessmentRequest& request)
{
    if (!nativeTimerInventoryEvidenceValid(evidence)
        || !nativeTimerAbsenceAssessmentRequestValid(request))
    {
        return NativeTimerAbsenceAssessmentStatus::invalid;
    }
    if (evidence.backendId != request.backendId)
        return NativeTimerAbsenceAssessmentStatus::backendConflict;
    if (evidence.backendGeneration != request.backendGeneration)
        return NativeTimerAbsenceAssessmentStatus::generationConflict;
    if (evidence.observedAt < request.notBefore)
        return NativeTimerAbsenceAssessmentStatus::staleEvidence;

    const bool found = std::binary_search(
        evidence.backendNativeTimerIds.begin(),
        evidence.backendNativeTimerIds.end(),
        request.backendNativeTimerId);
    return found
        ? NativeTimerAbsenceAssessmentStatus::present
        : NativeTimerAbsenceAssessmentStatus::absent;
}

}
