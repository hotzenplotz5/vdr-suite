#pragma once

#include "TimerAssignmentPlanner.h"
#include "TimerAssignmentRepository.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{
class NativeTimerBindingRepository;
class TimerIntentRepository;

enum class TimerAssignmentReassignmentStatus
{
    persisted,
    alreadyPersisted,
    invalidRequest,
    replacementIdConflict,
    intentNotFound,
    intentRevisionConflict,
    assignmentNotFound,
    assignmentRevisionConflict,
    assignmentEpochConflict,
    assignmentStateConflict,
    backendConflict,
    generationConflict,
    assignmentSetConflict,
    nativeOutcomeUnsafe,
    bindingNotFound,
    bindingConflict,
    operationNotFound,
    operationConflict,
    planningInvalid,
    noEligibleBackend,
    ownershipConflict,
    repositoryError,
};

struct TimerAssignmentReassignmentRequest
{
    std::string replacementTimerAssignmentId;
    std::string oldTimerAssignmentId;
    std::string expectedOldAssignmentRevision;
    std::uint64_t expectedOldAssignmentEpoch = 0;
    std::string expectedIntentRevision;
    std::string expectedOldBackendId;
    std::uint64_t expectedOldBackendGeneration = 0;
    TimerAssignmentReassignmentNativeOutcome oldNativeOutcome =
        TimerAssignmentReassignmentNativeOutcome::beforeDispatch;
    std::string oldOperationId;
    std::string expectedOldOperationRevision;
    std::string oldNativeTimerBindingId;
    std::string expectedOldBindingRevision;
    std::string reason;
    std::int64_t createdAt = 0;
    std::vector<TimerAssignmentPlanningBackendCandidate> candidates;
};

struct TimerAssignmentReassignmentResult
{
    TimerAssignmentReassignmentStatus status =
        TimerAssignmentReassignmentStatus::repositoryError;
    TimerAssignmentPlanningDecision decision;
    TimerAssignment oldAssignment;
    TimerAssignment replacementAssignment;
    TimerAssignmentReassignmentEvidence evidence;

    bool ok() const
    {
        return status == TimerAssignmentReassignmentStatus::persisted
            || status == TimerAssignmentReassignmentStatus::alreadyPersisted;
    }
};

class TimerAssignmentReassignmentService
{
public:
    TimerAssignmentReassignmentService(
        TimerIntentRepository& intentRepository,
        TimerAssignmentRepository& assignmentRepository,
        NativeTimerBindingRepository& bindingRepository,
        vdrsuite::operations::MutationOperationRepository& operationRepository);

    TimerAssignmentReassignmentResult reassign(
        const TimerAssignmentReassignmentRequest& request);

private:
    TimerIntentRepository& intentRepository_;
    TimerAssignmentRepository& assignmentRepository_;
    NativeTimerBindingRepository& bindingRepository_;
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
};

const char* timerAssignmentReassignmentStatusName(
    TimerAssignmentReassignmentStatus status);

} // namespace vdrsuite::timers
