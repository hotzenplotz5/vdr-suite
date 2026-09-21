#pragma once

#include "NativeTimerBinding.h"
#include "NativeTimerObservation.h"
#include "NativeTimerReadbackExpectation.h"

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerPresentReadbackVerificationStatus
{
    verified,
    alreadyVerified,
    bindingNotFound,
    bindingRevisionConflict,
    identityConflict,
    ownershipConflict,
    generationConflict,
    staleObservation,
    reconciliationRequired,
    repositoryConflict,
    repositoryError,
    invalid,
};

struct NativeTimerPresentReadbackVerificationResult
{
    NativeTimerPresentReadbackVerificationStatus status =
        NativeTimerPresentReadbackVerificationStatus::repositoryError;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == NativeTimerPresentReadbackVerificationStatus::verified
            || status == NativeTimerPresentReadbackVerificationStatus::alreadyVerified;
    }
};

class NativeTimerPresentReadbackVerificationService
{
public:
    explicit NativeTimerPresentReadbackVerificationService(
        NativeTimerBindingRepository& repository);

    NativeTimerPresentReadbackVerificationResult verify(
        const NativeTimerReadbackExpectation& expectation,
        const NativeTimerObservation& observation);

private:
    NativeTimerBindingRepository& repository_;
};

}
