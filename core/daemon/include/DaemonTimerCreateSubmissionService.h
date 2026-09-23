#pragma once

#include "MutationOperation.h"
#include "NativeTimerSpecification.h"

#include <string>

class BackendAgentRepository;

namespace vdrsuite::agent
{
class BackendAgentNativeTimerCreateActivationService;
class BackendAgentNativeTimerCreateReservationService;
}

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{
class NativeTimerCreateDispatchService;
class NativeTimerCreateOperationPreparationService;
class TimerAssignmentRepository;
class TimerIntentRepository;
}

enum class DaemonTimerCreateReplayStatus
{
    notFound,
    matched,
    idempotencyConflict,
    invalid,
    unavailable,
};

struct DaemonTimerCreateReplayRequest
{
    std::string timerAssignmentId;
    std::string backendId;
    std::string actorId;
    std::string idempotencyKey;
    vdrsuite::timers::NativeTimerSpecification requestedSpecification;
};

struct DaemonTimerCreateReplayResult
{
    DaemonTimerCreateReplayStatus status =
        DaemonTimerCreateReplayStatus::unavailable;
    std::string expectedAssignmentRevision;
};

enum class DaemonTimerCreateSubmissionStatus
{
    accepted,
    invalid,
    notFound,
    revisionConflict,
    idempotencyConflict,
    stateConflict,
    generationConflict,
    backendUnavailable,
    capabilityUnavailable,
    unavailable,
};

struct DaemonTimerCreateSubmissionRequest
{
    std::string timerAssignmentId;
    std::string backendId;
    std::string actorId;
    std::string idempotencyKey;
    std::string expectedAssignmentRevision;
    vdrsuite::timers::NativeTimerSpecification requestedSpecification;
    std::string requestId;
    std::string correlationId;
};

struct DaemonTimerCreateSubmissionResult
{
    DaemonTimerCreateSubmissionStatus status =
        DaemonTimerCreateSubmissionStatus::unavailable;
    vdrsuite::operations::MutationOperation operation;

    bool ok() const
    {
        return status == DaemonTimerCreateSubmissionStatus::accepted;
    }
};

class DaemonTimerCreateSubmissionService
{
public:
    DaemonTimerCreateSubmissionService(
        vdrsuite::timers::TimerIntentRepository& intentRepository,
        vdrsuite::timers::TimerAssignmentRepository& assignmentRepository,
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        vdrsuite::timers::NativeTimerCreateOperationPreparationService&
            preparationService,
        BackendAgentRepository& agentRepository,
        vdrsuite::agent::BackendAgentNativeTimerCreateReservationService&
            reservationService,
        vdrsuite::timers::NativeTimerCreateDispatchService& dispatchService,
        vdrsuite::agent::BackendAgentNativeTimerCreateActivationService&
            activationService);

    DaemonTimerCreateReplayResult lookupReplay(
        const DaemonTimerCreateReplayRequest& request);

    DaemonTimerCreateSubmissionResult submit(
        const DaemonTimerCreateSubmissionRequest& request);

private:
    vdrsuite::timers::TimerIntentRepository& intentRepository_;
    vdrsuite::timers::TimerAssignmentRepository& assignmentRepository_;
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    vdrsuite::timers::NativeTimerCreateOperationPreparationService&
        preparationService_;
    BackendAgentRepository& agentRepository_;
    vdrsuite::agent::BackendAgentNativeTimerCreateReservationService&
        reservationService_;
    vdrsuite::timers::NativeTimerCreateDispatchService& dispatchService_;
    vdrsuite::agent::BackendAgentNativeTimerCreateActivationService&
        activationService_;
};
