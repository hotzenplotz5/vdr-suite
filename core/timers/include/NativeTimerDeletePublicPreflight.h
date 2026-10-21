#pragma once

#include "NativeTimerBinding.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

// Read-only public-boundary preflight, NOT authorization or dispatch authority.
// Upstream must verify grants and provider lease. observedNotBefore is a
// server-owned freshness floor, never a field from an untrusted client.
enum class PublicNativeTimerDeletePreflightStatus
{
    eligible,
    invalidRequest,
    invalidEvidence,
    backendConflict,
    nativeIdentityConflict,
    generationConflict,
    revisionConflict,
    fingerprintConflict,
    staleObservation,
    nativeStateConflict,
    adoptionRequired,
    ownershipConflict,
};

struct PublicNativeTimerDeletePreflightRequest
{
    std::string nativeTimerBindingId;
    std::string backendId;
    std::uint64_t expectedBackendGeneration = 0;
    std::string backendNativeTimerId;
    std::string expectedBindingRevision;
    std::string expectedObservedFingerprint;
    std::int64_t observedNotBefore = 0;
};

// Requires independently resolved durable NativeTimerBinding facts. No Timer
// index, client payload, or existing list row alone grants deletion authority.
inline PublicNativeTimerDeletePreflightStatus
publicNativeTimerDeletePreflight(
    const PublicNativeTimerDeletePreflightRequest& request,
    const NativeTimerBinding& binding)
{
    using S = PublicNativeTimerDeletePreflightStatus;
    if (request.nativeTimerBindingId.empty() ||
        request.backendId.empty() ||
        request.expectedBackendGeneration == 0 ||
        request.backendNativeTimerId.empty() ||
        request.expectedBindingRevision.empty() ||
        request.expectedObservedFingerprint.empty() ||
        request.observedNotBefore <= 0)
        return S::invalidRequest;
    if (!nativeTimerBindingValid(binding))
        return S::invalidEvidence;
    if (binding.backendId != request.backendId)
        return S::backendConflict;
    if (binding.nativeTimerBindingId != request.nativeTimerBindingId ||
        binding.backendNativeTimerId != request.backendNativeTimerId)
        return S::nativeIdentityConflict;
    if (binding.backendGeneration != request.expectedBackendGeneration)
        return S::generationConflict;
    if (!nativeTimerBindingRevisionMatches(
            request.expectedBindingRevision, binding.bindingRevision))
        return S::revisionConflict;
    if (binding.observedFingerprint != request.expectedObservedFingerprint)
        return S::fingerprintConflict;
    if (binding.lastObservedAt < request.observedNotBefore)
        return S::staleObservation;
    if (binding.missingSince != 0 ||
        binding.driftState != NativeTimerBindingDriftState::none ||
        binding.observedState.recording ||
        binding.observedState.pending)
        return S::nativeStateConflict;

    // Phase-64 dispatch and Agent contracts require a TimerAssignment.
    // External Timer ownership must never be silently promoted.
    if (binding.ownership == NativeTimerBindingOwnership::external)
        return S::adoptionRequired;
    if (binding.ownership != NativeTimerBindingOwnership::managed &&
        binding.ownership != NativeTimerBindingOwnership::adopted)
        return S::ownershipConflict;
    if (binding.timerAssignmentId.empty())
        return S::invalidEvidence;
    return S::eligible;
}

} // namespace vdrsuite::timers
