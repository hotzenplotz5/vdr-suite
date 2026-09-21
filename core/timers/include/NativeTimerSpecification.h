#pragma once

#include "NativeTimerBinding.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

struct NativeTimerSpecification
{
    std::string channelId;
    std::string title;
    std::string directory;
    std::string day;
    std::string weekdays = "-------";
    std::string startTime;
    std::string endTime;
    std::int32_t priority = 50;
    std::int32_t lifetime = 99;
    bool enabled = true;
    bool vps = false;
};

bool nativeTimerSpecificationValid(
    const NativeTimerSpecification& specification);
std::string nativeTimerSpecificationFingerprint(
    const NativeTimerSpecification& specification);
bool nativeTimerObservationMatchesSpecification(
    const NativeTimerSpecification& specification,
    const NativeTimerObservedState& observedState);

}
