#pragma once

#include "MutationOperation.h"
#include "NativeTimerCreateOperationPayload.h"

#include <cstdint>
#include <string>

class Database;

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{

class NativeTimerCreateOperationPreparationService;
class TimerAssignmentFulfillmentService;
class TimerAssignmentRepository;

enum class NativeTimerCreateAdmissionStatus
{
    prepared,
    replayed,
    assignmentNotFound,
    assignmentRevisionConflict,
    assignmentStateConflict,
    backendConflict,
    specificationUnavailable,
    intentRevisionConflict,
    generationConflict,
    idempotencyConflict,
    operationConflict,
    identityGenerationFailed,
    repositoryError,
    invalid,
};

struct NativeTimerCreateAdmissionRequest
{
    std::string actorId;
    std::string idempotencyKey;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedBackendId;
    std::int64_t requestedAt = 0;
    std::int64_t deadline = 0;
};

struct NativeTimerCreateAdmissionResult
{
    NativeTimerCreateAdmissionStatus status =
        NativeTimerCreateAdmissionStatus::repositoryError;
    vdrsuite::operations::MutationOperation operation;
    NativeTimerCreateOperationPayload payload;

    bool ok() const
    {
        return status == NativeTimerCreateAdmissionStatus::prepared
            || status == NativeTimerCreateAdmissionStatus::replayed;
    }
};

class NativeTimerCreateAdmissionService
{
public:
    NativeTimerCreateAdmissionService(
        Database& database,
        TimerAssignmentRepository& assignmentRepository,
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        TimerAssignmentFulfillmentService& fulfillmentService,
        NativeTimerCreateOperationPreparationService& preparationService);

    NativeTimerCreateAdmissionResult admit(
        const NativeTimerCreateAdmissionRequest& request);

private:
    NativeTimerCreateAdmissionResult replayIfPresent(
        const NativeTimerCreateAdmissionRequest& request,
        const std::string& requestFingerprint,
        bool& present);

    Database& database_;
    TimerAssignmentRepository& assignmentRepository_;
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    TimerAssignmentFulfillmentService& fulfillmentService_;
    NativeTimerCreateOperationPreparationService& preparationService_;
};

} // namespace vdrsuite::timers
