#pragma once

#include "NativeTimerAbsenceReadbackExpectation.h"
#include "NativeTimerBinding.h"
#include "NativeTimerInventoryEvidence.h"

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerAbsenceReadbackVerificationStatus
{
    verified,
    alreadyVerified,
    bindingNotFound,
    bindingRevisionConflict,
    identityConflict,
    ownershipConflict,
    generationConflict,
    staleEvidence,
    reconciliationRequired,
    repositoryConflict,
    repositoryError,
    invalid,
};

struct NativeTimerAbsenceReadbackVerificationResult
{
    NativeTimerAbsenceReadbackVerificationStatus status =
        NativeTimerAbsenceReadbackVerificationStatus::repositoryError;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == NativeTimerAbsenceReadbackVerificationStatus::verified
            || status == NativeTimerAbsenceReadbackVerificationStatus::alreadyVerified;
    }
};

class NativeTimerAbsenceReadbackVerificationService
{
public:
    explicit NativeTimerAbsenceReadbackVerificationService(
        NativeTimerBindingRepository& repository);

    NativeTimerAbsenceReadbackVerificationResult verify(
        const NativeTimerAbsenceReadbackExpectation& expectation,
        const NativeTimerInventoryEvidence& evidence);

private:
    NativeTimerBindingRepository& repository_;
};

}
