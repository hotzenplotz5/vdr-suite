#pragma once

#include "NativeTimerReadbackExpectation.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

// Backend-neutral ADR-0042 expectation for one native Timer to be ABSENT after
// an operation. This value is operation intent/evidence only; it does not prove
// absence. Verification still requires a complete generation-fenced inventory.
struct NativeTimerAbsenceReadbackExpectation
{
    std::string operationId;
    NativeTimerReadbackOperationState operationState =
        NativeTimerReadbackOperationState::executedUnverified;

    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;

    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;

    std::int64_t readbackNotBefore = 0;
};

bool nativeTimerAbsenceReadbackExpectationValid(
    const NativeTimerAbsenceReadbackExpectation& expectation);

}
