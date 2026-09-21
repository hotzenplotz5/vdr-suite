#pragma once

#include "MutationOperation.h"
#include "NativeTimerCreateReadbackExpectation.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

struct NativeTimerCreateCommandReservationReference
{
    std::string commandId;
    std::string requestFingerprint;
};

bool nativeTimerCreateCommandReservationReferenceValid(
    const NativeTimerCreateCommandReservationReference& reference);
std::string serializeNativeTimerCreateCommandReservationReference(
    const NativeTimerCreateCommandReservationReference& reference);
bool parseNativeTimerCreateCommandReservationReference(
    const std::string& serialized,
    NativeTimerCreateCommandReservationReference& reference);

enum class NativeTimerCreateDispatchClaimStatus
{
    claimed,
    alreadyClaimed,
    operationNotFound,
    payloadNotFound,
    operationStateConflict,
    operationRevisionConflict,
    identityConflict,
    payloadConflict,
    deadlineExpired,
    operationRepositoryError,
    invalid,
};

struct NativeTimerCreateDispatchClaimRequest
{
    std::string operationId;
    std::string expectedOperationRevision;
    std::string timerAssignmentId;
    std::string nativeTimerBindingId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string expectedSpecificationFingerprint;
    NativeTimerCreateCommandReservationReference reservation;
};

struct NativeTimerCreateDispatchClaimResult
{
    NativeTimerCreateDispatchClaimStatus status =
        NativeTimerCreateDispatchClaimStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerCreateCommandReservationReference reservation;

    bool ok() const
    {
        return status == NativeTimerCreateDispatchClaimStatus::claimed ||
            status == NativeTimerCreateDispatchClaimStatus::alreadyClaimed;
    }
};

enum class NativeTimerCreateExecutorOutcomeCategory
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct NativeTimerCreateExecutorOutcome
{
    std::string operationId;
    std::string operationRevision;
    NativeTimerCreateCommandReservationReference reservation;
    NativeTimerCreateExecutorOutcomeCategory category =
        NativeTimerCreateExecutorOutcomeCategory::outcomeUnknown;
    std::int64_t dispatchStartedAt = 0;
    std::int64_t completedAt = 0;
    std::string evidenceReference;
};

enum class NativeTimerCreateDispatchOutcomeStatus
{
    applied,
    alreadyApplied,
    operationNotFound,
    payloadNotFound,
    identityConflict,
    payloadConflict,
    operationStateConflict,
    operationRevisionConflict,
    operationRepositoryError,
    invalid,
};

struct NativeTimerCreateDispatchOutcomeResult
{
    NativeTimerCreateDispatchOutcomeStatus status =
        NativeTimerCreateDispatchOutcomeStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    bool expectationPresent = false;
    NativeTimerCreateReadbackExpectation expectation;

    bool ok() const
    {
        return status == NativeTimerCreateDispatchOutcomeStatus::applied ||
            status == NativeTimerCreateDispatchOutcomeStatus::alreadyApplied;
    }
};

class NativeTimerCreateDispatchService
{
public:
    explicit NativeTimerCreateDispatchService(
        vdrsuite::operations::MutationOperationRepository& operationRepository);

    NativeTimerCreateDispatchClaimResult claimAfterReservation(
        const NativeTimerCreateDispatchClaimRequest& request,
        std::int64_t claimedAt);

    NativeTimerCreateDispatchOutcomeResult applyOutcome(
        const NativeTimerCreateExecutorOutcome& outcome);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
};

} // namespace vdrsuite::timers
