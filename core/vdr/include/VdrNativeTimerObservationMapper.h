#pragma once

#include "NativeTimerObservation.h"
#include "VdrTimer.h"

#include <cstdint>
#include <string>

enum class VdrNativeTimerObservationMapStatus
{
    ok,
    invalidBackendIdentity,
    invalidBackendGeneration,
    invalidObservedAt,
    invalidNativeTimerIdentity,
    invalidObservedState,
};

struct VdrNativeTimerObservationMapResult
{
    VdrNativeTimerObservationMapStatus status =
        VdrNativeTimerObservationMapStatus::invalidObservedState;
    vdrsuite::timers::NativeTimerObservation observation;

    bool ok() const
    {
        return status == VdrNativeTimerObservationMapStatus::ok;
    }
};

class VdrNativeTimerObservationMapper
{
public:
    static VdrNativeTimerObservationMapResult map(
        const std::string& backendId,
        std::uint64_t backendGeneration,
        std::int64_t observedAt,
        const VdrTimer& timer);
};
