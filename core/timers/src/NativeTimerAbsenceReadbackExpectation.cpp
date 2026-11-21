#include "NativeTimerAbsenceReadbackExpectation.h"

#include <cstddef>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;

bool nonEmptyBounded(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
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

bool nativeTimerAbsenceReadbackExpectationValid(
    const NativeTimerAbsenceReadbackExpectation& expectation)
{
    return nonEmptyBounded(expectation.operationId)
        && operationStateValid(expectation.operationState)
        && nonEmptyBounded(expectation.nativeTimerBindingId)
        && nonEmptyBounded(expectation.expectedBindingRevision)
        && nonEmptyBounded(expectation.backendId)
        && expectation.backendGeneration != 0
        && nonEmptyBounded(expectation.backendNativeTimerId)
        && expectation.readbackNotBefore > 0;
}

}
