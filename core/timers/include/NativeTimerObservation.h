#pragma once

#include "NativeTimerBinding.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

// Backend-neutral evidence for one authoritative present native Timer read.
// The backend/runtime generation is supplied by the caller's lifecycle
// authority; it is never inferred from adapter-local state.
struct NativeTimerObservation
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::int64_t observedAt = 0;
    NativeTimerObservedState observedState;
    std::string observedFingerprint;
};

bool nativeTimerObservationValid(const NativeTimerObservation& observation);

}
