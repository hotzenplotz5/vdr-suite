#pragma once

#include "MutationOperation.h"
#include "NativeTimerAbsenceReadbackExpectation.h"
#include "NativeTimerDeleteOperationPreparationService.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerDeleteDispatchClaimStatus
{
    claimed,
    alreadyClaimed,
    operationNotFound,
    bindingNotFound,
    identityConflict,
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

struct NativeTimerDeleteDispatchClaim
{
    std::string operationId;
    std::string operationRevision;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::int64_t claimedAt = 0;
};

struct NativeTimerDeleteDispatchClaimResult
{
    NativeTimerDeleteDispatchClaimStatus status =
        NativeTimerDeleteDispatchClaimStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerDeleteDispatchClaim claim;

    bool ok() const
    {
        return status == NativeTimerDeleteDispatchClaimStatus::claimed ||
            status == NativeTimerDeleteDispatchClaimStatus::alreadyClaimed;
    }
};

enum class NativeTimerDeleteExecutorOutcomeCategory
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct NativeTimerDeleteExecutorOutcome
{
    NativeTimerDeleteExecutorOutcomeCategory category =
        NativeTimerDeleteExecutorOutcomeCategory::outcomeUnknown;
    // Positive only when a native dispatch may have begun. This is the
    // authoritative lower bound for later readback eligibility.
    std::int64_t dispatchStartedAt = 0;
    std::int64_t completedAt = 0;
    std::string evidenceReference;
};

enum class NativeTimerDeleteDispatchOutcomeStatus
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

struct NativeTimerDeleteDispatchOutcomeResult
{
    NativeTimerDeleteDispatchOutcomeStatus status =
        NativeTimerDeleteDispatchOutcomeStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    bool expectationPresent = false;
    NativeTimerAbsenceReadbackExpectation expectation;

    bool ok() const
    {
        return status == NativeTimerDeleteDispatchOutcomeStatus::applied ||
            status == NativeTimerDeleteDispatchOutcomeStatus::alreadyApplied;
    }
};

class NativeTimerDeleteDispatchService
{
public:
    NativeTimerDeleteDispatchService(
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        NativeTimerBindingRepository& bindingRepository);

    NativeTimerDeleteDispatchClaimResult claim(
        const NativeTimerDeleteDispatchHandoff& handoff,
        std::int64_t claimedAt);

    NativeTimerDeleteDispatchOutcomeResult applyOutcome(
        const NativeTimerDeleteDispatchClaim& claim,
        const NativeTimerDeleteExecutorOutcome& outcome);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    NativeTimerBindingRepository& bindingRepository_;
};

}
