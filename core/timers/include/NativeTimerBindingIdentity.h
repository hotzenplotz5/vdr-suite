#pragma once

#include <string>

namespace vdrsuite::timers
{

// Canonical Control-Plane identity for newly pre-reserved managed
// NativeTimerBinding correlations. Existing durable binding IDs retain their
// established compatibility validity.
std::string generateNativeTimerBindingId();
bool nativeTimerBindingIdCanonical(const std::string& value);

} // namespace vdrsuite::timers
