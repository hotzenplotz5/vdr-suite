#include "NativeTimerCreateReadbackEvidence.h"

#include <cstddef>
#include <set>
#include <string>

namespace vdrsuite::timers
{
namespace
{

constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxCandidates = 4096;

bool validIdentity(const std::string& value)
{
    return !value.empty()
        && value.size() <= kMaxIdentityLength;
}

}

bool nativeTimerCreateReadbackCandidateValid(
    const NativeTimerCreateReadbackCandidate& candidate)
{
    return validIdentity(candidate.timerAssignmentId)
        && validIdentity(candidate.nativeTimerBindingId)
        && nativeTimerObservationValid(candidate.observation);
}

bool nativeTimerCreateReadbackEvidenceValid(
    const NativeTimerCreateReadbackEvidence& evidence)
{
    if (!validIdentity(evidence.backendId)
        || evidence.backendGeneration == 0
        || evidence.observedAt <= 0
        || evidence.completeness
            != NativeTimerCreateReadbackCompleteness::complete
        || evidence.candidates.size() > kMaxCandidates)
    {
        return false;
    }

    std::set<std::string> nativeTimerIds;

    for (const auto& candidate : evidence.candidates)
    {
        if (!nativeTimerCreateReadbackCandidateValid(candidate)
            || candidate.observation.backendId != evidence.backendId
            || candidate.observation.backendGeneration
                != evidence.backendGeneration
            || candidate.observation.observedAt != evidence.observedAt
            || !nativeTimerIds.insert(
                candidate.observation.backendNativeTimerId).second)
        {
            return false;
        }
    }

    return true;
}

}
