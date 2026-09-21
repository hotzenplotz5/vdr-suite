#pragma once

#include "MutationOperation.h"
#include "NativeTimerBinding.h"
#include "NativeTimerCreateReadbackExpectation.h"
#include "TimerAssignment.h"

#include <cstdint>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;
class TimerAssignmentRepository;

enum class NativeTimerCreateOperationCompletionStatus
{
    completed,
    alreadyCompleted,
    operationNotFound,
    assignmentNotFound,
    bindingNotFound,
    identityConflict,
    verificationPolicyConflict,
    operationStateConflict,
    verificationEvidenceMissing,
    operationRevisionConflict,
    operationRepositoryError,
    assignmentRepositoryError,
    bindingRepositoryError,
    invalid,
};

struct NativeTimerCreateOperationCompletionResult
{
    NativeTimerCreateOperationCompletionStatus status =
        NativeTimerCreateOperationCompletionStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    TimerAssignment assignment;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status ==
                NativeTimerCreateOperationCompletionStatus::completed ||
            status ==
                NativeTimerCreateOperationCompletionStatus::alreadyCompleted;
    }
};

class NativeTimerCreateOperationCompletionService
{
public:
    NativeTimerCreateOperationCompletionService(
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        TimerAssignmentRepository& assignmentRepository,
        NativeTimerBindingRepository& bindingRepository);

    NativeTimerCreateOperationCompletionResult complete(
        const NativeTimerCreateReadbackExpectation& expectation,
        std::int64_t completedAt);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    TimerAssignmentRepository& assignmentRepository_;
    NativeTimerBindingRepository& bindingRepository_;
};

} // namespace vdrsuite::timers
