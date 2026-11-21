#pragma once

#include "NativeTimerSpecification.h"

#include <string>

namespace vdrsuite::timers
{

std::string serializeTimerAssignmentDesiredNativeTimerSpecification(
    const NativeTimerSpecification& specification);

bool parseTimerAssignmentDesiredNativeTimerSpecification(
    const std::string& encoded,
    NativeTimerSpecification& specification);

bool timerAssignmentDesiredNativeTimerSpecificationEquivalent(
    const NativeTimerSpecification& left,
    const NativeTimerSpecification& right);

} // namespace vdrsuite::timers
