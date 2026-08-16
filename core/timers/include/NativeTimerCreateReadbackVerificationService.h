#pragma once

#include "NativeTimerBinding.h"
#include "NativeTimerCreateReadbackEvidence.h"
#include "NativeTimerCreateReadbackExpectation.h"

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerCreateReadbackVerificationStatus
{
    verified,
    alreadyVerified,
    correlationNotFound,
    correlationAmbiguous,
    backendConflict,
    generationConflict,
    staleEvidence,
    reconciliationRequired,
    bindingConflict,
    nativeIdentityConflict,
    assignmentBindingConflict,
    repositoryError,
    invalid,
};

struct NativeTimerCreateReadbackVerificationResult
{
    NativeTimerCreateReadbackVerificationStatus status =
        NativeTimerCreateReadbackVerificationStatus::repositoryError;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == NativeTimerCreateReadbackVerificationStatus::verified
            || status == NativeTimerCreateReadbackVerificationStatus::alreadyVerified;
    }
};

class NativeTimerCreateReadbackVerificationService
{
public:
    explicit NativeTimerCreateReadbackVerificationService(
        NativeTimerBindingRepository& repository);

    NativeTimerCreateReadbackVerificationResult verify(
        const NativeTimerCreateReadbackExpectation& expectation,
        const NativeTimerCreateReadbackEvidence& evidence);

private:
    NativeTimerBindingRepository& repository_;
};

}
