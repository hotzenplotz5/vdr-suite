#pragma once

#include "MutationOperation.h"
#include "NativeTimerAbsenceReadbackExpectation.h"
#include "NativeTimerBinding.h"

#include <cstdint>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerDeleteOperationCompletionStatus
{
    completed,
    alreadyCompleted,
    operationNotFound,
    bindingNotFound,
    identityConflict,
    verificationPolicyConflict,
    operationStateConflict,
    verificationEvidenceMissing,
    operationRevisionConflict,
    operationRepositoryError,
    bindingRepositoryError,
    invalid,
};

struct NativeTimerDeleteOperationCompletionResult
{
    NativeTimerDeleteOperationCompletionStatus status =
        NativeTimerDeleteOperationCompletionStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == NativeTimerDeleteOperationCompletionStatus::completed ||
            status == NativeTimerDeleteOperationCompletionStatus::alreadyCompleted;
    }
};

class NativeTimerDeleteOperationCompletionService
{
public:
    NativeTimerDeleteOperationCompletionService(
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        NativeTimerBindingRepository& bindingRepository);

    NativeTimerDeleteOperationCompletionResult complete(
        const NativeTimerAbsenceReadbackExpectation& expectation,
        std::int64_t completedAt);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    NativeTimerBindingRepository& bindingRepository_;
};

}
