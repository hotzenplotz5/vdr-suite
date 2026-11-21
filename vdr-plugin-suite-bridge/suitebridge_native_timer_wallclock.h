#pragma once

#include <ctime>

namespace suitebridge_native_timer_wallclock
{

inline bool sameCivilMinute(const std::tm& left, const std::tm& right)
{
    return left.tm_year == right.tm_year &&
           left.tm_mon == right.tm_mon &&
           left.tm_mday == right.tm_mday &&
           left.tm_hour == right.tm_hour &&
           left.tm_min == right.tm_min;
}

// A VDR day+HHMM Timer has no DST-fold bit. Both civil-time resolutions
// must not identify distinct epochs. The VDR process's own timezone is
// authoritative; this function never touches VDR timers/devices.
inline bool uniquelyRepresentsMinute(
    std::time_t instant, const std::tm& native)
{
    if (native.tm_isdst != 0 && native.tm_isdst != 1)
        return false;
    std::tm summer = native, winter = native;
    summer.tm_isdst = 1;
    winter.tm_isdst = 0;
    const std::time_t summerInstant = std::mktime(&summer);
    const std::time_t winterInstant = std::mktime(&winter);
    if (summerInstant == static_cast<std::time_t>(-1) ||
        winterInstant == static_cast<std::time_t>(-1))
        return false;

    std::tm summerRoundTrip{}, winterRoundTrip{};
    if (!localtime_r(&summerInstant, &summerRoundTrip) ||
        !localtime_r(&winterInstant, &winterRoundTrip))
        return false;

    const bool summerMatches = sameCivilMinute(native, summerRoundTrip);
    const bool winterMatches = sameCivilMinute(native, winterRoundTrip);
    if (summerInstant != winterInstant &&
        summerMatches && winterMatches)
        return false;

    // Reject libc normalization failures or a mismatch to the requested
    // instant. Unknown timezone behavior is unavailable, not safe.
    return (summerInstant == instant && summerMatches) ||
           (winterInstant == instant && winterMatches);
}

} // namespace suitebridge_native_timer_wallclock
