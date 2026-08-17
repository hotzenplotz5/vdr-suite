#pragma once

#include "MutationOperation.h"
#include "NativeTimerModifyReadbackVerificationService.h"

#include <cstdint>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerModifyOperationCompletionStatus
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

struct NativeTimerModifyOperationCompletionResult
{
    NativeTimerModifyOperationCompletionStatus status =
        NativeTimerModifyOperationCompletionStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status ==
                NativeTimerModifyOperationCompletionStatus::completed ||
            status ==
                NativeTimerModifyOperationCompletionStatus::alreadyCompleted;
    }
};

class NativeTimerModifyOperationCompletionService
{
public:
    NativeTimerModifyOperationCompletionService(
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        NativeTimerBindingRepository& bindingRepository);

    NativeTimerModifyOperationCompletionResult complete(
        const NativeTimerModifyReadbackExpectation& expectation,
        std::int64_t completedAt);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    NativeTimerBindingRepository& bindingRepository_;
};

} // namespace vdrsuite::timers
