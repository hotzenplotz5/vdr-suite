#pragma once

#include "MutationOperation.h"

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

class NativeTimerCreateDispatchService
{
public:
    explicit NativeTimerCreateDispatchService(
        vdrsuite::operations::MutationOperationRepository& operationRepository);

    NativeTimerCreateDispatchClaimResult claimAfterReservation(
        const NativeTimerCreateDispatchClaimRequest& request,
        std::int64_t claimedAt);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
};

} // namespace vdrsuite::timers
