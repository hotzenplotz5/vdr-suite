#pragma once

#include "NativeTimerBinding.h"
#include "VdrTimer.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

// Read-only evidence for a future single-Timer GET. An ETag is *not*
// permission to DELETE, nor a substitute for current Agent/provider fences.
enum class NativeTimerPublicDetailRevisionStatus
{
    verifiedManaged,
    verifiedAdopted,
    adoptionRequired,
    unboundReadOnly,
    unsafeEvidence,
};

struct NativeTimerPublicDetailRevision
{
    NativeTimerPublicDetailRevisionStatus status =
        NativeTimerPublicDetailRevisionStatus::unsafeEvidence;
    std::string resourceRevision;
};

// The caller must obtain currentGeneration from an authenticated,
// independently inspected BackendAgent, never from the HTTP client.
// A snapshot-only native Timer (including an unknown external one)
// remains readable, but cannot yield a write-precondition ETag.
inline NativeTimerPublicDetailRevision nativeTimerPublicDetailRevision(
    const std::string& backendId,
    std::uint64_t currentGeneration,
    const VdrTimer& timer,
    const NativeTimerBinding* binding)
{
    using S = NativeTimerPublicDetailRevisionStatus;
    NativeTimerPublicDetailRevision result;
    if (backendId.empty() || backendId.size() > 160U ||
        currentGeneration == 0 ||
        timer.id.empty() || timer.id.size() > 160U)
        return result;

    if (binding == nullptr)
    {
        result.status = S::unboundReadOnly;
        return result;
    }

    if (!nativeTimerBindingValid(*binding) ||
        binding->backendId != backendId ||
        binding->backendNativeTimerId != timer.id ||
        binding->backendGeneration != currentGeneration ||
        binding->missingSince != 0 ||
        binding->driftState != NativeTimerBindingDriftState::none ||
        timer.recording || timer.pending)
        return result;

    NativeTimerObservedState observed;
    observed.channelId = timer.channelId;
    observed.eventId = timer.eventId;
    observed.title = timer.title;
    observed.directory = timer.directory;
    observed.day = timer.day;
    observed.weekdays = timer.weekdays;
    observed.startTime = timer.startTime;
    observed.endTime = timer.endTime;
    observed.flags = timer.flags;
    observed.priority = timer.priority;
    observed.lifetime = timer.lifetime;
    observed.enabled = timer.enabled;
    observed.vps = timer.vps;
    observed.recording = timer.recording;
    observed.pending = timer.pending;
    if (nativeTimerObservedStateFingerprint(observed) !=
        binding->observedFingerprint)
        return result;

    switch (binding->ownership)
    {
        case NativeTimerBindingOwnership::managed:
            result.status = S::verifiedManaged;
            break;
        case NativeTimerBindingOwnership::adopted:
            result.status = S::verifiedAdopted;
            break;
        case NativeTimerBindingOwnership::external:
            result.status = S::adoptionRequired;
            break;
        case NativeTimerBindingOwnership::orphanedManaged:
        case NativeTimerBindingOwnership::ambiguous:
            return result;
    }

    // Scoped to one durable binding and its observed backend generation.
    // A future HTTP endpoint may wrap this with publicStrongEntityTag().
    result.resourceRevision = "native-timer-binding/1:" +
        binding->nativeTimerBindingId + ":" +
        std::to_string(binding->backendGeneration) + ":" +
        binding->bindingRevision;
    return result;
}

} // namespace vdrsuite::timers
