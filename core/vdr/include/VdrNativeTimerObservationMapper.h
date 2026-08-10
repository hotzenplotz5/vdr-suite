#pragma once

#include "NativeTimerBinding.h"
#include "VdrTimer.h"

#include <cstdint>
#include <string>

struct VdrNativeTimerObservation
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::int64_t observedAt = 0;
    vdrsuite::timers::NativeTimerObservedState observedState;
    std::string observedFingerprint;
};

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
    VdrNativeTimerObservation observation;

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
