#pragma once

#include "TimerAssignmentPlanner.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::timers
{

enum class TimerAssignmentSchedulingStatus
{
    persisted,
    alreadyPersisted,
    activePrimaryExists,
    replicaTargetSatisfied,
    invalidRequest,
    assignmentIdConflict,
    intentNotFound,
    intentRevisionConflict,
    planningInvalid,
    ownershipConflict,
    assignmentSetConflict,
    repositoryConflict,
    storageError
};

struct TimerAssignmentPrimarySchedulingRequest
{
    std::string timerAssignmentId;
    std::string timerIntentId;
    std::string expectedIntentRevision;
    std::int64_t createdAt = 0;
    std::vector<TimerAssignmentPlanningBackendCandidate> candidates;
};

struct TimerAssignmentReplicaSchedulingRequest
{
    std::string timerAssignmentId;
    std::string timerIntentId;
    std::string expectedIntentRevision;
    std::int64_t createdAt = 0;
    std::vector<TimerAssignmentPlanningBackendCandidate> candidates;
};

struct TimerAssignmentSchedulingResult
{
    TimerAssignmentSchedulingStatus status =
        TimerAssignmentSchedulingStatus::storageError;
    TimerAssignmentPlanningDecision decision;
    TimerAssignment assignment;

    bool ok() const
    {
        return status == TimerAssignmentSchedulingStatus::persisted
            || status == TimerAssignmentSchedulingStatus::alreadyPersisted
            || status == TimerAssignmentSchedulingStatus::activePrimaryExists
            || status == TimerAssignmentSchedulingStatus::replicaTargetSatisfied;
    }
};

class TimerAssignmentSchedulingService
{
public:
    TimerAssignmentSchedulingService(
        TimerIntentRepository& intentRepository,
        TimerAssignmentRepository& assignmentRepository);

    TimerAssignmentSchedulingResult schedulePrimary(
        const TimerAssignmentPrimarySchedulingRequest& request);

    TimerAssignmentSchedulingResult scheduleReplica(
        const TimerAssignmentReplicaSchedulingRequest& request);

private:
    TimerIntentRepository& intentRepository_;
    TimerAssignmentRepository& assignmentRepository_;
};

const char* timerAssignmentSchedulingStatusName(
    TimerAssignmentSchedulingStatus status);

}
