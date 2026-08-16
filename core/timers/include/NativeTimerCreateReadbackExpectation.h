#pragma once

#include "NativeTimerReadbackExpectation.h"
#include "NativeTimerSpecification.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

// CREATE-specific authoritative PRESENT-readback expectation.
//
// Unlike update/toggle readback, no backendNativeTimerId or binding revision
// exists yet. The stable Suite binding identity is reserved before dispatch;
// the backend-native identity is learned only from authoritative correlated
// readback after the native create may have executed.
struct NativeTimerCreateReadbackExpectation
{
    std::string operationId;

    NativeTimerReadbackOperationState operationState =
        NativeTimerReadbackOperationState::executedUnverified;

    std::string timerAssignmentId;
    std::string nativeTimerBindingId;

    std::string backendId;
    std::uint64_t backendGeneration = 0;

    std::int64_t readbackNotBefore = 0;

    NativeTimerSpecification expectedSpecification;
    std::string expectedSpecificationFingerprint;
};

bool nativeTimerCreateReadbackExpectationValid(
    const NativeTimerCreateReadbackExpectation& expectation);

}
