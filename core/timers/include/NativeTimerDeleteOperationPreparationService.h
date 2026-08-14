#pragma once

#include "MutationOperation.h"
#include "NativeTimerBinding.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerDeleteOperationPreparationStatus
{
    prepared,
    alreadyPrepared,
    bindingNotFound,
    bindingRevisionConflict,
    generationConflict,
    ownershipConflict,
    bindingMissing,
    driftConflict,
    idempotencyConflict,
    operationConflict,
    operationStateConflict,
    operationRepositoryError,
    bindingRepositoryError,
    invalid,
};

struct NativeTimerDeleteOperationPreparationRequest
{
    std::string operationId;
    std::string idempotencyKey;
    std::string actorId;
    std::string requestFingerprint;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::uint64_t expectedBackendGeneration = 0;
    std::int64_t requestedAt = 0;
    std::int64_t deadline = 0;
};

// Immutable pre-dispatch target context. This is intentionally not a
// NativeTimerAbsenceReadbackExpectation: no backend mutation has happened yet,
// so neither an unresolved post-dispatch operation state nor readbackNotBefore
// may be manufactured here.
struct NativeTimerDeleteDispatchHandoff
{
    std::string operationId;
    std::string operationRevision;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string timerAssignmentId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
};

struct NativeTimerDeleteOperationPreparationResult
{
    NativeTimerDeleteOperationPreparationStatus status =
        NativeTimerDeleteOperationPreparationStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerBinding binding;
    NativeTimerDeleteDispatchHandoff handoff;

    bool ok() const
    {
        return status == NativeTimerDeleteOperationPreparationStatus::prepared ||
            status == NativeTimerDeleteOperationPreparationStatus::alreadyPrepared;
    }
};

class NativeTimerDeleteOperationPreparationService
{
public:
    NativeTimerDeleteOperationPreparationService(
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        NativeTimerBindingRepository& bindingRepository);

    NativeTimerDeleteOperationPreparationResult prepare(
        const NativeTimerDeleteOperationPreparationRequest& request);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    NativeTimerBindingRepository& bindingRepository_;
};

}
