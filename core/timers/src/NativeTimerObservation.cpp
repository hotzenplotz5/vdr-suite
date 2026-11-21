#include "NativeTimerObservation.h"

#include <cstddef>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}
}

bool nativeTimerObservationValid(const NativeTimerObservation& observation)
{
    if (!validIdentity(observation.backendId)
        || observation.backendGeneration == 0
        || !validIdentity(observation.backendNativeTimerId)
        || observation.observedAt <= 0
        || !nativeTimerObservedStateValid(observation.observedState))
    {
        return false;
    }

    const std::string fingerprint =
        nativeTimerObservedStateFingerprint(observation.observedState);
    return !fingerprint.empty()
        && observation.observedFingerprint == fingerprint;
}

}
