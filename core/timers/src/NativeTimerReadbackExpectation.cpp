#include "NativeTimerReadbackExpectation.h"

#include <cstddef>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxFingerprintLength = 4096;

bool nonEmptyBounded(const std::string& value, std::size_t maximum)
{
    return !value.empty() && value.size() <= maximum;
}

bool operationStateValid(NativeTimerReadbackOperationState state)
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

const char* nativeTimerReadbackOperationStateName(
    NativeTimerReadbackOperationState state)
{
    switch (state)
    {
        case NativeTimerReadbackOperationState::executedUnverified:
            return "executed_unverified";
        case NativeTimerReadbackOperationState::outcomeUnknown:
            return "outcome_unknown";
    }
    return "invalid";
}

bool nativeTimerReadbackExpectationValid(
    const NativeTimerReadbackExpectation& expectation)
{
    if (!nonEmptyBounded(expectation.operationId, kMaxIdentityLength)
        || !operationStateValid(expectation.operationState)
        || !nonEmptyBounded(
            expectation.nativeTimerBindingId,
            kMaxIdentityLength)
        || !nonEmptyBounded(
            expectation.expectedBindingRevision,
            kMaxIdentityLength)
        || !nonEmptyBounded(expectation.backendId, kMaxIdentityLength)
        || expectation.backendGeneration == 0
        || !nonEmptyBounded(
            expectation.backendNativeTimerId,
            kMaxIdentityLength)
        || expectation.readbackNotBefore <= 0
        || !nativeTimerObservedStateValid(expectation.expectedState)
        || !nonEmptyBounded(
            expectation.expectedFingerprint,
            kMaxFingerprintLength))
    {
        return false;
    }

    return expectation.expectedFingerprint
        == nativeTimerObservedStateFingerprint(expectation.expectedState);
}

}
