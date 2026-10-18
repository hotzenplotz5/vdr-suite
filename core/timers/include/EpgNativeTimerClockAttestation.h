#pragma once

#include "EpgNativeTimerExecutionAuthority.h"
#include "EpgNativeTimerTimeGuard.h"
#include "SuiteBridgeNativeTimerCreateTransport.h"

#include <cstdint>
#include <string>

namespace vdrsuite::public_epg_timer
{
// Convert a *verified* native CLOCK reply into server-owned evidence. The
// caller must first use SuiteBridgeNativeTimerCreateTransport::
// verifyNativeClockReply with a reply from the selected backend's VDR.
// This does not perform I/O and cannot authorize CREATE or conflict clearance.
inline bool attestBackendNativeTimerClock(
    const vdrsuite::agent::SuiteBridgeNativeTimerClockProjection& verified,
    const BackendAgentRecord& agent,
    const vdrsuite::agent::BackendAgentLocalProviderSelection& provider,
    const vdrsuite::timers::TimerAssignmentPlanningBackendCandidate& candidate,
    std::int64_t expectedStart, std::int64_t expectedStop,
    std::int64_t observedAt,
    ServerOwnedNativeClockEvidence& evidence)
{
    evidence = {};
    if (expectedStart <= 0 || expectedStop <= expectedStart ||
        observedAt <= 0 ||
        verified.start != expectedStart || verified.stop != expectedStop ||
        verified.providerInstanceEpoch != provider.providerInstanceEpoch ||
        !currentNativeTimerCreateExecutionAuthority(
            agent, provider, candidate, observedAt))
        return false;

    // Verify timestamp, offset and both local projections once more before
    // attaching Agent generation and Provider ownership. Native CLOCK
    // returns minute-truncated HHMM; STOP is rounded up only when building
    // the separate native CREATE specification.
    std::string startDay, startHhmm, stopDay, stopHhmm;
    if (!projectEpgBoundaryToNative(
            expectedStart, verified.startUtcOffsetSeconds,
            false, startDay, startHhmm) ||
        !projectEpgBoundaryToNative(
            expectedStop, verified.stopUtcOffsetSeconds,
            false, stopDay, stopHhmm) ||
        startDay != verified.startDay ||
        startHhmm != verified.startHhmm ||
        stopDay != verified.stopDay ||
        stopHhmm != verified.stopHhmm)
        return false;

    evidence.current = true;
    evidence.startUtcOffsetSeconds = verified.startUtcOffsetSeconds;
    evidence.stopUtcOffsetSeconds = verified.stopUtcOffsetSeconds;
    evidence.backendId = candidate.backendId;
    evidence.backendGeneration = candidate.currentBackendGeneration;
    evidence.providerInstanceEpoch = provider.providerInstanceEpoch;
    evidence.executionAuthorityFence = candidate.executionAuthorityFence;
    evidence.observedAt = observedAt;
    return true;
}
} // namespace vdrsuite::public_epg_timer
