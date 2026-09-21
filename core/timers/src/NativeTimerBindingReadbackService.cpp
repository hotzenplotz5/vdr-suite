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
    const NativeTimerObservation& observation)
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
    if (observation.backendGeneration < current.backendGeneration)
        return result(
            NativeTimerBindingReadbackStatus::staleGeneration,
            current);
    if (observation.observedAt < current.lastObservedAt)
        return result(
            NativeTimerBindingReadbackStatus::staleObservation,
            current);

    // A present observation cannot silently clear durable missing evidence, and
    // a material fingerprint change must remain available to reconciliation.
    if (current.missingSince != 0
        || observation.observedFingerprint != current.observedFingerprint)
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

    // Preserve the already-durable copied representation when the normalized
    // fingerprint is equal. This prevents 930/0930 representation churn from
    // issuing revisions while still advancing authoritative observation fences.
    const auto updated = repository_.update(next, current.bindingRevision);
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
