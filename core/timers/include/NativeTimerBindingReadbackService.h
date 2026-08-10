#pragma once

#include "NativeTimerBinding.h"
#include "NativeTimerObservation.h"

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerBindingReadbackStatus
{
    refreshed,
    alreadyCurrent,
    unboundObservation,
    staleGeneration,
    staleObservation,
    reconciliationRequired,
    repositoryConflict,
    repositoryError,
    invalid,
};

struct NativeTimerBindingReadbackResult
{
    NativeTimerBindingReadbackStatus status =
        NativeTimerBindingReadbackStatus::repositoryError;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == NativeTimerBindingReadbackStatus::refreshed
            || status == NativeTimerBindingReadbackStatus::alreadyCurrent;
    }
};

class NativeTimerBindingReadbackService
{
public:
    explicit NativeTimerBindingReadbackService(
        NativeTimerBindingRepository& repository);

    // Applies only semantically unchanged present observations. Changed or
    // previously-missing state is returned for reconciliation instead of being
    // overwritten. There is no implicit discovery/adoption and no hidden retry
    // after an optimistic-concurrency conflict.
    NativeTimerBindingReadbackResult applyPresentObservation(
        const NativeTimerObservation& observation);

private:
    NativeTimerBindingRepository& repository_;
};

}
