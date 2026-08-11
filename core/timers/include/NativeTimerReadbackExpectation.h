#pragma once

#include "NativeTimerBinding.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

enum class NativeTimerReadbackOperationState
{
    executedUnverified,
    outcomeUnknown
};

// Backend-neutral ADR-0042 evidence describing one expected PRESENT native
// Timer readback. Native absence is deliberately a separate contract because
// transport failure or an incomplete snapshot is not proof of deletion.
struct NativeTimerReadbackExpectation
{
    std::string operationId;
    NativeTimerReadbackOperationState operationState =
        NativeTimerReadbackOperationState::executedUnverified;

    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;

    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;

    // Authoritative observations older than this boundary cannot verify the
    // operation, even when their state happens to match the expected result.
    std::int64_t readbackNotBefore = 0;

    NativeTimerObservedState expectedState;
    std::string expectedFingerprint;
};

const char* nativeTimerReadbackOperationStateName(
    NativeTimerReadbackOperationState state);

bool nativeTimerReadbackExpectationValid(
    const NativeTimerReadbackExpectation& expectation);

}
