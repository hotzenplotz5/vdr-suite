#include "NativeTimerCreateReadbackExpectation.h"

#include <cstddef>

namespace vdrsuite::timers
{
namespace
{

constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxFingerprintLength = 4096;

bool nonEmptyBounded(
    const std::string& value,
    std::size_t maximum)
{
    return !value.empty() && value.size() <= maximum;
}

bool operationStateValid(
    NativeTimerReadbackOperationState state)
{
    switch (state)
    {
        case NativeTimerReadbackOperationState::executedUnverified:
        case NativeTimerReadbackOperationState::outcomeUnknown:
            return true;
    }

    return false;
}

}

bool nativeTimerCreateReadbackExpectationValid(
    const NativeTimerCreateReadbackExpectation& expectation)
{
    if (!nonEmptyBounded(
            expectation.operationId,
            kMaxIdentityLength)
        || !operationStateValid(expectation.operationState)
        || !nonEmptyBounded(
            expectation.timerAssignmentId,
            kMaxIdentityLength)
        || !nonEmptyBounded(
            expectation.nativeTimerBindingId,
            kMaxIdentityLength)
        || !nonEmptyBounded(
            expectation.backendId,
            kMaxIdentityLength)
        || expectation.backendGeneration == 0
        || expectation.readbackNotBefore <= 0
        || !nativeTimerSpecificationValid(
            expectation.expectedSpecification)
        || !nonEmptyBounded(
            expectation.expectedSpecificationFingerprint,
            kMaxFingerprintLength))
    {
        return false;
    }

    return expectation.expectedSpecificationFingerprint
        == nativeTimerSpecificationFingerprint(
            expectation.expectedSpecification);
}

}
