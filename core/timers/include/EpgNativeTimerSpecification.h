#pragma once

#include "EpgNativeTimerTimeGuard.h"

#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::public_epg_timer
{

// Constructs a native one-shot exclusively from authoritative server/VDR
// facts. Caller must supply a backend-fenced, typed CLOCK attestation;
// a blank or foreign time proof cannot be replaced by daemon-local TZ.
// Actual CREATE authorization and prospective conflict proof are separate.
inline bool makeServerOwnedEpgNativeOneShot(
    const AuthoritativeObservation& event,
    const ServerOwnedRecordingDefaults& defaults,
    const ServerOwnedNativeClockEvidence& clock,
    const std::string& authoritativeTitle,
    const std::string& serverOwnedDirectory,
    vdrsuite::timers::NativeTimerSpecification& native)
{
    native = {};
    if (!clock.current || !event.exactNativeIdentity ||
        event.backendId.empty() ||
        clock.backendId != event.backendId ||
        clock.backendGeneration == 0 ||
        clock.backendGeneration != event.backendGeneration ||
        clock.providerInstanceEpoch.empty() ||
        clock.executionAuthorityFence.empty() ||
        clock.observedAt <= 0 ||
        defaults.vpsPreferred ||  // native isolated preview rejects VPS
        defaults.startMarginSeconds < 0 ||
        defaults.stopMarginSeconds < 0 ||
        event.startTime <= defaults.startMarginSeconds ||
        event.endTime <= event.startTime ||
        event.endTime > std::numeric_limits<std::int64_t>::max() -
            defaults.stopMarginSeconds ||
        authoritativeTitle.empty() || authoritativeTitle.size() > 512U ||
        serverOwnedDirectory.size() > 512U)
        return false;

    const std::int64_t start =
        event.startTime - defaults.startMarginSeconds;
    const std::int64_t stop =
        event.endTime + defaults.stopMarginSeconds;
    std::string startDay, startTime, stopDay, stopTime;
    if (!projectEpgBoundaryToNative(
            start, clock.startUtcOffsetSeconds, false,
            startDay, startTime) ||
        !projectEpgBoundaryToNative(
            stop, clock.stopUtcOffsetSeconds, true,
            stopDay, stopTime))
        return false;

    vdrsuite::timers::NativeTimerSpecification candidate;
    candidate.channelId = event.channelId;
    candidate.title = authoritativeTitle;
    candidate.directory = serverOwnedDirectory;
    candidate.day = startDay;
    candidate.weekdays = "-------";
    candidate.startTime = startTime;
    candidate.endTime = stopTime;
    candidate.priority = defaults.priority;
    candidate.lifetime = defaults.lifetimeDays;
    candidate.enabled = true;
    candidate.vps = false;
    if (!vdrsuite::timers::nativeTimerSpecificationValid(candidate) ||
        !nativeOneShotMatchesEpg(event, defaults, clock, candidate))
        return false;

    native = std::move(candidate);
    return true;
}

} // namespace vdrsuite::public_epg_timer
