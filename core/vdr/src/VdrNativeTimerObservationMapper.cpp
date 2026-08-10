#include "VdrNativeTimerObservationMapper.h"

#include <cstddef>

namespace
{
constexpr std::size_t kMaxIdentityLength = 160;

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

VdrNativeTimerObservationMapResult statusResult(
    VdrNativeTimerObservationMapStatus status)
{
    VdrNativeTimerObservationMapResult result;
    result.status = status;
    return result;
}
}

VdrNativeTimerObservationMapResult VdrNativeTimerObservationMapper::map(
    const std::string& backendId,
    std::uint64_t backendGeneration,
    std::int64_t observedAt,
    const VdrTimer& timer)
{
    if (!validIdentity(backendId))
        return statusResult(
            VdrNativeTimerObservationMapStatus::invalidBackendIdentity);

    if (backendGeneration == 0)
    {
        return statusResult(
            VdrNativeTimerObservationMapStatus::invalidBackendGeneration);
    }

    if (observedAt <= 0)
        return statusResult(
            VdrNativeTimerObservationMapStatus::invalidObservedAt);

    if (!validIdentity(timer.id))
        return statusResult(
            VdrNativeTimerObservationMapStatus::invalidNativeTimerIdentity);

    VdrNativeTimerObservationMapResult result;
    result.status = VdrNativeTimerObservationMapStatus::ok;
    result.observation.backendId = backendId;
    result.observation.backendGeneration = backendGeneration;
    result.observation.backendNativeTimerId = timer.id;
    result.observation.observedAt = observedAt;

    auto& state = result.observation.observedState;
    state.channelId = timer.channelId;
    state.eventId = timer.eventId;
    state.title = timer.title;
    state.directory = timer.directory;
    state.day = timer.day;
    state.weekdays = timer.weekdays;
    state.startTime = timer.startTime;
    state.endTime = timer.endTime;
    state.flags = timer.flags;
    state.priority = timer.priority;
    state.lifetime = timer.lifetime;
    state.enabled = timer.enabled;
    state.vps = timer.vps;
    state.recording = timer.recording;
    state.pending = timer.pending;

    if (!vdrsuite::timers::nativeTimerObservedStateValid(state))
        return statusResult(
            VdrNativeTimerObservationMapStatus::invalidObservedState);

    result.observation.observedFingerprint =
        vdrsuite::timers::nativeTimerObservedStateFingerprint(state);
    if (result.observation.observedFingerprint.empty())
        return statusResult(
            VdrNativeTimerObservationMapStatus::invalidObservedState);

    return result;
}
