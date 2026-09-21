#pragma once

#include "MutationOperation.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerSpecification.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class TimerAssignmentRepository;
class TimerIntentRepository;

enum class NativeTimerCreateOperationPreparationStatus
{
    prepared,
    alreadyPrepared,
    intentNotFound,
    assignmentNotFound,
    intentRevisionConflict,
    intentStateConflict,
    assignmentRevisionConflict,
    assignmentEpochConflict,
    assignmentStateConflict,
    backendConflict,
    generationConflict,
    channelConflict,
    bindingAlreadyPresent,
    idempotencyConflict,
    operationConflict,
    operationStateConflict,
    operationRepositoryError,
    invalid,
};

struct NativeTimerCreateOperationPreparationRequest
{
    std::string operationId;
    std::string idempotencyKey;
    std::string actorId;
    std::string requestFingerprint;

    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t expectedAssignmentEpoch = 0;
    std::string nativeTimerBindingId;

    std::string expectedBackendId;
    std::uint64_t expectedBackendGeneration = 0;

    NativeTimerSpecification expectedSpecification;

    std::int64_t requestedAt = 0;
    std::int64_t deadline = 0;
};

struct NativeTimerCreateOperationPreparationResult
{
    NativeTimerCreateOperationPreparationStatus status =
        NativeTimerCreateOperationPreparationStatus::operationRepositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerCreateOperationPayload payload;

    bool ok() const
    {
        return status == NativeTimerCreateOperationPreparationStatus::prepared
            || status == NativeTimerCreateOperationPreparationStatus::alreadyPrepared;
    }
};

class NativeTimerCreateOperationPreparationService
{
public:
    NativeTimerCreateOperationPreparationService(
        TimerIntentRepository& intentRepository,
        TimerAssignmentRepository& assignmentRepository,
        vdrsuite::operations::MutationOperationRepository& operationRepository);

    NativeTimerCreateOperationPreparationResult prepare(
        const NativeTimerCreateOperationPreparationRequest& request);

private:
    TimerIntentRepository& intentRepository_;
    TimerAssignmentRepository& assignmentRepository_;
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
};

}
