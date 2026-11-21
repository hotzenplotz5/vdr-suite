#pragma once

#include "MutationOperation.h"
#include "NativeTimerModifyReadbackVerificationService.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerModifyDispatchClaimStatus
{
    claimed,
    alreadyClaimed,
    operationNotFound,
    payloadNotFound,
    bindingNotFound,
    identityConflict,
    payloadConflict,
    operationStateConflict,
    operationRevisionConflict,
    bindingRevisionConflict,
    generationConflict,
    ownershipConflict,
    bindingStateConflict,
    operationRepositoryError,
    bindingRepositoryError,
    invalid,
};

struct NativeTimerModifyDispatchClaimRequest
{
    std::string operationId;
    std::string expectedOperationRevision;
};

struct NativeTimerModifyDispatchClaim
{
    std::string operationId;
    std::string operationRevision;
    NativeTimerModifyOperationPayload payload;
    std::int64_t claimedAt = 0;
};

struct NativeTimerModifyDispatchClaimResult
{
    NativeTimerModifyDispatchClaimStatus status =
        NativeTimerModifyDispatchClaimStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerModifyDispatchClaim claim;

    bool ok() const
    {
        return status == NativeTimerModifyDispatchClaimStatus::claimed ||
            status == NativeTimerModifyDispatchClaimStatus::alreadyClaimed;
    }
};

enum class NativeTimerModifyExecutorOutcomeCategory
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct NativeTimerModifyExecutorOutcome
{
    NativeTimerModifyExecutorOutcomeCategory category =
        NativeTimerModifyExecutorOutcomeCategory::outcomeUnknown;
    std::int64_t dispatchStartedAt = 0;
    std::int64_t completedAt = 0;
    std::string evidenceReference;
};

enum class NativeTimerModifyDispatchOutcomeStatus
{
    applied,
    alreadyApplied,
    operationNotFound,
    identityConflict,
    operationStateConflict,
    operationRevisionConflict,
    operationRepositoryError,
    invalid,
};

struct NativeTimerModifyDispatchOutcomeResult
{
    NativeTimerModifyDispatchOutcomeStatus status =
        NativeTimerModifyDispatchOutcomeStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    bool expectationPresent = false;
    NativeTimerModifyReadbackExpectation expectation;

    bool ok() const
    {
        return status == NativeTimerModifyDispatchOutcomeStatus::applied ||
            status == NativeTimerModifyDispatchOutcomeStatus::alreadyApplied;
    }
};

class NativeTimerModifyDispatchService
{
public:
    NativeTimerModifyDispatchService(
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        NativeTimerBindingRepository& bindingRepository);

    NativeTimerModifyDispatchClaimResult claim(
        const NativeTimerModifyDispatchClaimRequest& request,
        std::int64_t claimedAt);

    NativeTimerModifyDispatchOutcomeResult applyOutcome(
        const NativeTimerModifyDispatchClaim& claim,
        const NativeTimerModifyExecutorOutcome& outcome);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    NativeTimerBindingRepository& bindingRepository_;
};

} // namespace vdrsuite::timers
