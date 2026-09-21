#pragma once

#include "MutationOperation.h"
#include "NativeTimerModifyOperationPayload.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;
class TimerAssignmentRepository;
class TimerIntentRepository;

enum class NativeTimerModifyOperationPreparationStatus
{
    prepared,
    alreadyPrepared,
    intentNotFound,
    assignmentNotFound,
    bindingNotFound,
    intentRevisionConflict,
    intentStateConflict,
    assignmentRevisionConflict,
    assignmentEpochConflict,
    assignmentStateConflict,
    bindingRevisionConflict,
    identityConflict,
    ownershipConflict,
    generationConflict,
    currentFingerprintConflict,
    bindingStateConflict,
    toggleShapeConflict,
    operationStateConflict,
    idempotencyConflict,
    operationConflict,
    repositoryError,
    invalid,
};

struct NativeTimerModifyOperationPreparationRequest
{
    std::string operationId;
    std::string idempotencyKey;
    std::string actorId;
    std::string requestFingerprint;
    NativeTimerModifyKind kind = NativeTimerModifyKind::update;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t expectedAssignmentEpoch = 0;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string expectedBackendId;
    std::uint64_t expectedBackendGeneration = 0;
    std::string backendNativeTimerId;
    std::string expectedCurrentFingerprint;
    NativeTimerSpecification expectedSpecification;
    std::int64_t requestedAt = 0;
    std::int64_t deadline = 0;
};

struct NativeTimerModifyOperationPreparationResult
{
    NativeTimerModifyOperationPreparationStatus status =
        NativeTimerModifyOperationPreparationStatus::repositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerModifyOperationPayload payload;

    bool ok() const
    {
        return status == NativeTimerModifyOperationPreparationStatus::prepared
            || status ==
                NativeTimerModifyOperationPreparationStatus::alreadyPrepared;
    }
};

class NativeTimerModifyOperationPreparationService
{
public:
    NativeTimerModifyOperationPreparationService(
        TimerIntentRepository& intentRepository,
        TimerAssignmentRepository& assignmentRepository,
        NativeTimerBindingRepository& bindingRepository,
        vdrsuite::operations::MutationOperationRepository& operationRepository);

    NativeTimerModifyOperationPreparationResult prepare(
        const NativeTimerModifyOperationPreparationRequest& request);

private:
    TimerIntentRepository& intentRepository_;
    TimerAssignmentRepository& assignmentRepository_;
    NativeTimerBindingRepository& bindingRepository_;
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
};

}
