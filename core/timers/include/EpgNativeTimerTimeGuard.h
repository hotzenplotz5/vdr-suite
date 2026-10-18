#pragma once

#include "EpgTimerIntentDraft.h"
#include "NativeTimerSpecification.h"

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <limits>
#include <string>

namespace vdrsuite::public_epg_timer
{
// Offsets apply to the margin-adjusted boundaries independently. The
// caller must attest these from the owning Backend/VDR clock, including DST.
// They are never accepted from a paired Device request.
struct ServerOwnedNativeClockEvidence
{
    bool current = false;
    std::int32_t startUtcOffsetSeconds = 0;
    std::int32_t stopUtcOffsetSeconds = 0;
};

inline bool projectEpgBoundaryToNative(
    std::int64_t unixSeconds, std::int32_t offsetSeconds,
    bool roundEndUp, std::string& date, std::string& hhmm)
{
    constexpr std::int32_t kMaximumUtcOffset = 14 * 60 * 60;
    if (unixSeconds <= 0 || offsetSeconds < -kMaximumUtcOffset ||
        offsetSeconds > kMaximumUtcOffset ||
        offsetSeconds % 60 != 0 ||
        unixSeconds > std::numeric_limits<std::int64_t>::max()
            - kMaximumUtcOffset - 60)
        return false;

    std::int64_t localSeconds = unixSeconds + offsetSeconds;
    if (localSeconds <= 0) return false;
    const std::int64_t remainder = localSeconds % 60;
    if (roundEndUp && remainder != 0)
        localSeconds += 60 - remainder;
    else if (!roundEndUp)
        localSeconds -= remainder;

    const auto timestamp = static_cast<std::time_t>(localSeconds);
    if (static_cast<std::int64_t>(timestamp) != localSeconds)
        return false;
    std::tm parts{};
    if (!gmtime_r(&timestamp, &parts) ||
        parts.tm_year + 1900 < 1970 ||
        parts.tm_year + 1900 > 9999)
        return false;
    char dateBuffer[16]{};
    char timeBuffer[8]{};
    if (std::snprintf(dateBuffer, sizeof(dateBuffer),
            "%04d-%02d-%02d", parts.tm_year + 1900,
            parts.tm_mon + 1, parts.tm_mday) != 10 ||
        std::snprintf(timeBuffer, sizeof(timeBuffer),
            "%02d%02d", parts.tm_hour, parts.tm_min) != 4)
        return false;
    date = dateBuffer;
    hhmm = timeBuffer;
    return true;
}

inline bool nativeOneShotMatchesEpg(
    const AuthoritativeObservation& event,
    const ServerOwnedRecordingDefaults& defaults,
    const ServerOwnedNativeClockEvidence& clock,
    const vdrsuite::timers::NativeTimerSpecification& native)
{
    // An absent backend-native offset proof is not UTC=0. A timer formatted
    // using the daemon's timezone could be wrong on a remote backend.
    // VDR's day+HHMM one-shot encoding does not contain a DST fold bit.
    // An event straddling an offset transition cannot yet be represented
    // unambiguously by the current native CREATE contract. Fail closed.
    if (!clock.current ||
        clock.startUtcOffsetSeconds != clock.stopUtcOffsetSeconds ||
        !event.exactNativeIdentity || !event.observationCurrent ||
        event.endTime <= event.startTime ||
        event.startTime <= 0 ||
        event.endTime - event.startTime > 24 * 60 * 60 ||
        defaults.timezone.empty() ||
        defaults.startMarginSeconds < 0 ||
        defaults.stopMarginSeconds < 0 ||
        defaults.startMarginSeconds > 24 * 60 * 60 ||
        defaults.stopMarginSeconds > 24 * 60 * 60 ||
        event.startTime <= defaults.startMarginSeconds ||
        event.endTime > std::numeric_limits<std::int64_t>::max() -
            defaults.stopMarginSeconds)
        return false;

    const auto start = event.startTime - defaults.startMarginSeconds;
    const auto stop = event.endTime + defaults.stopMarginSeconds;
    // One native date + two HHMM fields have no explicit stop date. VDR
    // infers same-day/next-day from those fields, so a >=24-hour window
    // would silently wrap or truncate even though the event itself is valid.
    // Bound the minute-rounded interval, not just the source event length.
    const auto nativeStart = start - start % 60;
    const auto nativeStop = stop + (60 - stop % 60) % 60;
    if (nativeStop <= nativeStart ||
        nativeStop - nativeStart >= 24 * 60 * 60)
        return false;

    std::string localDay, localStart, ignoredEndDay, localStop;
    if (!projectEpgBoundaryToNative(start, clock.startUtcOffsetSeconds,
            false, localDay, localStart) ||
        !projectEpgBoundaryToNative(stop, clock.stopUtcOffsetSeconds,
            true, ignoredEndDay, localStop))
        return false;

    return native.channelId == event.channelId
        && native.day == localDay
        && native.startTime == localStart
        && native.endTime == localStop
        && native.weekdays == "-------"
        && native.priority == defaults.priority
        && native.lifetime == defaults.lifetimeDays
        && native.vps == defaults.vpsPreferred
        && native.enabled;
}

} // namespace vdrsuite::public_epg_timer
