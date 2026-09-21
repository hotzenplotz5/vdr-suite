#pragma once

#include "NativeTimerBinding.h"
#include "TimerAssignment.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;
class TimerAssignmentRepository;

enum class TimerAssignmentFulfillmentStatus
{
    provisioningStarted,
    alreadyProvisioning,
    bound,
    alreadyBound,
    assignmentNotFound,
    bindingNotFound,
    assignmentRevisionConflict,
    bindingRevisionConflict,
    intentRevisionConflict,
    stateConflict,
    identityConflict,
    generationConflict,
    ownershipConflict,
    bindingStateConflict,
    repositoryConflict,
    repositoryError,
    invalid,
};

struct TimerAssignmentFulfillmentResult
{
    TimerAssignmentFulfillmentStatus status =
        TimerAssignmentFulfillmentStatus::repositoryError;
    TimerAssignment assignment;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == TimerAssignmentFulfillmentStatus::provisioningStarted
            || status == TimerAssignmentFulfillmentStatus::alreadyProvisioning
            || status == TimerAssignmentFulfillmentStatus::bound
            || status == TimerAssignmentFulfillmentStatus::alreadyBound;
    }
};

class TimerAssignmentFulfillmentService
{
public:
    TimerAssignmentFulfillmentService(
        TimerAssignmentRepository& assignmentRepository,
        NativeTimerBindingRepository& bindingRepository);

    TimerAssignmentFulfillmentResult beginProvisioning(
        const std::string& timerAssignmentId,
        const std::string& expectedAssignmentRevision,
        const std::string& expectedIntentRevision,
        std::uint64_t expectedBackendGeneration,
        std::int64_t updatedAt);

    TimerAssignmentFulfillmentResult bindVerified(
        const std::string& timerAssignmentId,
        const std::string& expectedAssignmentRevision,
        const std::string& expectedIntentRevision,
        std::uint64_t expectedBackendGeneration,
        const std::string& nativeTimerBindingId,
        const std::string& expectedBindingRevision,
        std::int64_t updatedAt);

private:
    TimerAssignmentRepository& assignmentRepository_;
    NativeTimerBindingRepository& bindingRepository_;
};

}
