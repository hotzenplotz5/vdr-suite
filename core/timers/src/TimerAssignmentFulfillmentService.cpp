#include "TimerAssignmentFulfillmentService.h"

#include "NativeTimerBindingRepository.h"
#include "TimerAssignmentRepository.h"

namespace vdrsuite::timers
{
namespace
{
TimerAssignmentFulfillmentResult result(
    TimerAssignmentFulfillmentStatus status,
    const TimerAssignment& assignment = {},
    const NativeTimerBinding& binding = {})
{
    TimerAssignmentFulfillmentResult value;
    value.status = status;
    value.assignment = assignment;
    value.binding = binding;
    return value;
}

bool validCommon(
    const std::string& timerAssignmentId,
    const std::string& expectedAssignmentRevision,
    const std::string& expectedIntentRevision,
    std::uint64_t expectedBackendGeneration,
    std::int64_t updatedAt)
{
    return !timerAssignmentId.empty()
        && !expectedAssignmentRevision.empty()
        && !expectedIntentRevision.empty()
        && expectedBackendGeneration > 0
        && updatedAt > 0;
}

TimerAssignmentFulfillmentStatus assignmentRepositoryFailure(
    TimerAssignmentRepositoryStatus status)
{
    switch (status)
    {
        case TimerAssignmentRepositoryStatus::conflict:
            return TimerAssignmentFulfillmentStatus::repositoryConflict;
        case TimerAssignmentRepositoryStatus::intentRevisionConflict:
            return TimerAssignmentFulfillmentStatus::intentRevisionConflict;
        case TimerAssignmentRepositoryStatus::ownershipConflict:
            return TimerAssignmentFulfillmentStatus::ownershipConflict;
        case TimerAssignmentRepositoryStatus::invalid:
            return TimerAssignmentFulfillmentStatus::invalid;
        default:
            return TimerAssignmentFulfillmentStatus::repositoryError;
    }
}

bool bindingVerifiedForAssignment(
    const NativeTimerBinding& binding,
    const TimerAssignment& assignment)
{
    return binding.timerAssignmentId == assignment.timerAssignmentId
        && binding.backendId == assignment.backendId
        && binding.backendGeneration == assignment.backendGeneration
        && binding.ownership == NativeTimerBindingOwnership::managed
        && !binding.lastVerifiedOperationId.empty()
        && binding.missingSince == 0
        && binding.driftState == NativeTimerBindingDriftState::none;
}
}

TimerAssignmentFulfillmentService::TimerAssignmentFulfillmentService(
    TimerAssignmentRepository& assignmentRepository,
    NativeTimerBindingRepository& bindingRepository)
    : assignmentRepository_(assignmentRepository),
      bindingRepository_(bindingRepository)
{
}

TimerAssignmentFulfillmentResult
TimerAssignmentFulfillmentService::beginProvisioning(
    const std::string& timerAssignmentId,
    const std::string& expectedAssignmentRevision,
    const std::string& expectedIntentRevision,
    std::uint64_t expectedBackendGeneration,
    std::int64_t updatedAt)
{
    if (!validCommon(
            timerAssignmentId,
            expectedAssignmentRevision,
            expectedIntentRevision,
            expectedBackendGeneration,
            updatedAt))
    {
        return result(TimerAssignmentFulfillmentStatus::invalid);
    }

    const auto found = assignmentRepository_.findById(timerAssignmentId);
    if (found.status == TimerAssignmentRepositoryStatus::notFound)
        return result(TimerAssignmentFulfillmentStatus::assignmentNotFound);
    if (!found.ok())
        return result(TimerAssignmentFulfillmentStatus::repositoryError);

    const TimerAssignment& current = found.assignment;
    if (current.intentRevision != expectedIntentRevision)
        return result(
            TimerAssignmentFulfillmentStatus::intentRevisionConflict,
            current);
    if (current.backendGeneration != expectedBackendGeneration)
        return result(
            TimerAssignmentFulfillmentStatus::generationConflict,
            current);
    if (current.assignmentRevision != expectedAssignmentRevision)
        return result(
            TimerAssignmentFulfillmentStatus::assignmentRevisionConflict,
            current);
    if (current.state == TimerAssignmentState::provisioning)
        return result(
            TimerAssignmentFulfillmentStatus::alreadyProvisioning,
            current);
    if (current.state != TimerAssignmentState::selected)
        return result(
            TimerAssignmentFulfillmentStatus::stateConflict,
            current);
    if (updatedAt <= current.updatedAt)
        return result(TimerAssignmentFulfillmentStatus::invalid, current);

    TimerAssignment next = current;
    next.state = TimerAssignmentState::provisioning;
    next.updatedAt = updatedAt;
    const auto updated = assignmentRepository_.update(
        next,
        current.assignmentRevision);
    if (updated.ok())
        return result(
            TimerAssignmentFulfillmentStatus::provisioningStarted,
            updated.assignment);

    return result(
        assignmentRepositoryFailure(updated.status),
        updated.assignment);
}

TimerAssignmentFulfillmentResult
TimerAssignmentFulfillmentService::bindVerified(
    const std::string& timerAssignmentId,
    const std::string& expectedAssignmentRevision,
    const std::string& expectedIntentRevision,
    std::uint64_t expectedBackendGeneration,
    const std::string& nativeTimerBindingId,
    const std::string& expectedBindingRevision,
    std::int64_t updatedAt)
{
    if (!validCommon(
            timerAssignmentId,
            expectedAssignmentRevision,
            expectedIntentRevision,
            expectedBackendGeneration,
            updatedAt)
        || nativeTimerBindingId.empty()
        || expectedBindingRevision.empty())
    {
        return result(TimerAssignmentFulfillmentStatus::invalid);
    }

    const auto foundAssignment =
        assignmentRepository_.findById(timerAssignmentId);
    if (foundAssignment.status == TimerAssignmentRepositoryStatus::notFound)
        return result(TimerAssignmentFulfillmentStatus::assignmentNotFound);
    if (!foundAssignment.ok())
        return result(TimerAssignmentFulfillmentStatus::repositoryError);

    const TimerAssignment& assignment = foundAssignment.assignment;
    if (assignment.intentRevision != expectedIntentRevision)
        return result(
            TimerAssignmentFulfillmentStatus::intentRevisionConflict,
            assignment);
    if (assignment.backendGeneration != expectedBackendGeneration)
        return result(
            TimerAssignmentFulfillmentStatus::generationConflict,
            assignment);

    const auto foundBinding =
        bindingRepository_.findById(nativeTimerBindingId);
    if (foundBinding.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(
            TimerAssignmentFulfillmentStatus::bindingNotFound,
            assignment);
    if (!foundBinding.ok())
        return result(
            TimerAssignmentFulfillmentStatus::repositoryError,
            assignment);

    const NativeTimerBinding& binding = foundBinding.binding;
    if (binding.bindingRevision != expectedBindingRevision)
        return result(
            TimerAssignmentFulfillmentStatus::bindingRevisionConflict,
            assignment,
            binding);
    if (binding.timerAssignmentId != assignment.timerAssignmentId
        || binding.backendId != assignment.backendId)
    {
        return result(
            TimerAssignmentFulfillmentStatus::identityConflict,
            assignment,
            binding);
    }
    if (binding.backendGeneration != expectedBackendGeneration)
        return result(
            TimerAssignmentFulfillmentStatus::generationConflict,
            assignment,
            binding);
    if (binding.ownership != NativeTimerBindingOwnership::managed)
        return result(
            TimerAssignmentFulfillmentStatus::ownershipConflict,
            assignment,
            binding);
    if (!bindingVerifiedForAssignment(binding, assignment))
        return result(
            TimerAssignmentFulfillmentStatus::bindingStateConflict,
            assignment,
            binding);

    if (assignment.state == TimerAssignmentState::bound)
    {
        if (assignment.nativeTimerBindingId == nativeTimerBindingId)
            return result(
                TimerAssignmentFulfillmentStatus::alreadyBound,
                assignment,
                binding);
        return result(
            TimerAssignmentFulfillmentStatus::identityConflict,
            assignment,
            binding);
    }

    if (assignment.assignmentRevision != expectedAssignmentRevision)
        return result(
            TimerAssignmentFulfillmentStatus::assignmentRevisionConflict,
            assignment,
            binding);
    if (assignment.state != TimerAssignmentState::provisioning
        && assignment.state != TimerAssignmentState::reconciling)
    {
        return result(
            TimerAssignmentFulfillmentStatus::stateConflict,
            assignment,
            binding);
    }
    if (updatedAt <= assignment.updatedAt)
        return result(
            TimerAssignmentFulfillmentStatus::invalid,
            assignment,
            binding);

    TimerAssignment next = assignment;
    next.state = TimerAssignmentState::bound;
    next.nativeTimerBindingId = nativeTimerBindingId;
    next.updatedAt = updatedAt;
    const auto updated = assignmentRepository_.update(
        next,
        assignment.assignmentRevision);
    if (updated.ok())
        return result(
            TimerAssignmentFulfillmentStatus::bound,
            updated.assignment,
            binding);

    return result(
        assignmentRepositoryFailure(updated.status),
        updated.assignment,
        binding);
}

}
