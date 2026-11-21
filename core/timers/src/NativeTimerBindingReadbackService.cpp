#include "NativeTimerBindingReadbackService.h"

#include "NativeTimerBindingRepository.h"

namespace vdrsuite::timers
{
namespace
{
NativeTimerBindingReadbackResult result(
    NativeTimerBindingReadbackStatus status,
    const NativeTimerBinding& binding = {})
{
    NativeTimerBindingReadbackResult value;
    value.status = status;
    value.binding = binding;
    return value;
}
}

NativeTimerBindingReadbackService::NativeTimerBindingReadbackService(
    NativeTimerBindingRepository& repository)
    : repository_(repository)
{
}

NativeTimerBindingReadbackResult
NativeTimerBindingReadbackService::applyPresentObservation(
    const NativeTimerObservation& observation,
    bool passiveFreshnessRefresh)
{
    if (!nativeTimerObservationValid(observation))
        return result(NativeTimerBindingReadbackStatus::invalid);

    const auto found = repository_.findByBackendNativeTimer(
        observation.backendId,
        observation.backendNativeTimerId);
    if (found.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(NativeTimerBindingReadbackStatus::unboundObservation);
    if (!found.ok())
        return result(NativeTimerBindingReadbackStatus::repositoryError);

    const NativeTimerBinding& current = found.binding;
    if (passiveFreshnessRefresh &&
        ((current.ownership != NativeTimerBindingOwnership::managed &&
          current.ownership != NativeTimerBindingOwnership::adopted) ||
         current.driftState != NativeTimerBindingDriftState::none ||
         current.missingSince != 0 ||
         current.observedState.recording || current.observedState.pending ||
         observation.backendGeneration < current.backendGeneration))
    {
        return result(
            NativeTimerBindingReadbackStatus::reconciliationRequired, current);
    }
    if (observation.backendGeneration < current.backendGeneration)
        return result(
            NativeTimerBindingReadbackStatus::staleGeneration,
            current);
    if (observation.observedAt < current.lastObservedAt)
        return result(
            NativeTimerBindingReadbackStatus::staleObservation,
            current);

    // Dynamic EPG association is not a timer configuration mutation. It may
    // change event_id from null to a schedule event after CREATE, even though
    // the native timer is unchanged. All other fields must match exactly.
    // The repository's passive-write path still fences ANY nonterminal timer
    // operation, so no in-flight UPDATE can lose its original fingerprint.
    const bool eventIdOnlyDrift =
        passiveFreshnessRefresh &&
        observation.observedFingerprint != current.observedFingerprint &&
        nativeTimerObservedStateSameExceptEventId(
            current.observedState, observation.observedState);
    if (current.missingSince != 0 ||
        (observation.observedFingerprint != current.observedFingerprint &&
         !eventIdOnlyDrift))
    {
        return result(
            NativeTimerBindingReadbackStatus::reconciliationRequired,
            current);
    }

    if (observation.backendGeneration == current.backendGeneration
        && observation.observedAt == current.lastObservedAt)
    {
        return result(
            NativeTimerBindingReadbackStatus::alreadyCurrent,
            current);
    }

    NativeTimerBinding next = current;
    next.backendGeneration = observation.backendGeneration;
    next.lastObservedAt = observation.observedAt;
    if (eventIdOnlyDrift)
    {
        next.observedState.eventId = observation.observedState.eventId;
        next.observedFingerprint = nativeTimerObservedStateFingerprint(
            next.observedState);
    }

    // Preserve existing copied clock/field representations when canonical
    // fingerprints are equal. Only an EPG-only difference is refreshed here;
    // all actual native Timer configuration changes remain reconciliation work.
    const auto updated = repository_.update(
        next, current.bindingRevision, passiveFreshnessRefresh);
    if (updated.status == NativeTimerBindingRepositoryStatus::conflict)
        return result(
            NativeTimerBindingReadbackStatus::repositoryConflict,
            updated.binding);
    if (!updated.ok())
        return result(NativeTimerBindingReadbackStatus::repositoryError);

    return result(
        NativeTimerBindingReadbackStatus::refreshed,
        updated.binding);
}

}
