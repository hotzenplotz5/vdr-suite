#pragma once

#include "NativeTimerModifyOperationPayload.h"
#include "NativeTimerObservation.h"

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerModifyReadbackVerificationStatus
{
    verified,
    alreadyVerified,
    bindingNotFound,
    bindingRevisionConflict,
    identityConflict,
    ownershipConflict,
    generationConflict,
    staleObservation,
    predecessorFingerprintConflict,
    reconciliationRequired,
    repositoryConflict,
    repositoryError,
    invalid,
};

struct NativeTimerModifyReadbackExpectation
{
    std::string operationId;
    NativeTimerModifyOperationPayload payload;
    std::int64_t readbackNotBefore = 0;
};

bool nativeTimerModifyReadbackExpectationValid(
    const NativeTimerModifyReadbackExpectation& expectation);

struct NativeTimerModifyReadbackVerificationResult
{
    NativeTimerModifyReadbackVerificationStatus status =
        NativeTimerModifyReadbackVerificationStatus::repositoryError;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == NativeTimerModifyReadbackVerificationStatus::verified
            || status ==
                NativeTimerModifyReadbackVerificationStatus::alreadyVerified;
    }
};

class NativeTimerModifyReadbackVerificationService
{
public:
    explicit NativeTimerModifyReadbackVerificationService(
        NativeTimerBindingRepository& repository);

    NativeTimerModifyReadbackVerificationResult verify(
        const NativeTimerModifyReadbackExpectation& expectation,
        const NativeTimerObservation& observation);

private:
    NativeTimerBindingRepository& repository_;
};

}
