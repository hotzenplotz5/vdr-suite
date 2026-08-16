#pragma once

#include "NativeTimerObservation.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::timers
{

enum class NativeTimerCreateReadbackCompleteness
{
    unknown,
    complete
};

// One native Timer carrying an explicitly decoded Suite ownership
// correlation. Backend-specific marker syntax is deliberately not exposed
// here; only stable Suite identities cross into the Timer domain.
struct NativeTimerCreateReadbackCandidate
{
    std::string timerAssignmentId;
    std::string nativeTimerBindingId;
    NativeTimerObservation observation;
};

// Evidence from one complete authoritative native Timer inventory.
//
// A reader must fail rather than mint complete evidence when the inventory,
// a reserved Suite marker or a correlated native observation cannot be parsed
// safely. Multiple candidates are retained so duplicate managed correlation
// is observable and cannot be silently collapsed.
struct NativeTimerCreateReadbackEvidence
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::int64_t observedAt = 0;

    NativeTimerCreateReadbackCompleteness completeness =
        NativeTimerCreateReadbackCompleteness::unknown;

    std::vector<NativeTimerCreateReadbackCandidate> candidates;
};

bool nativeTimerCreateReadbackCandidateValid(
    const NativeTimerCreateReadbackCandidate& candidate);

bool nativeTimerCreateReadbackEvidenceValid(
    const NativeTimerCreateReadbackEvidence& evidence);

}
