#include "TimerIntentApplicationService.h"

#include "NativeTimerBindingRepository.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

namespace vdrsuite::timers
{
namespace
{
TimerIntentApplicationResult result(
    TimerIntentApplicationStatus status,
    const TimerIntent& intent = {},
    const TimerAssignment& assignment = {})
{
    TimerIntentApplicationResult value;
    value.status = status;
    value.intent = intent;
    value.assignment = assignment;
    return value;
}

bool requestValid(const TimerIntentApplicationRequest& request)
{
    return timerIntentValid(request.intent)
        && request.intent.state == TimerIntentState::draft
        && request.intent.intentRevision.empty()
        && !request.timerAssignmentId.empty()
        && request.timerAssignmentId.size() <= 160
        && request.activatedAt > request.intent.updatedAt
        && request.scheduledAt > request.activatedAt
        && request.provisioningAt > request.scheduledAt;
}

bool sameIntentRequest(
    const TimerIntent& current,
    const TimerIntent& requested)
{
    return current.timerIntentId == requested.timerIntentId
        && current.createdByActorId == requested.createdByActorId
        && current.createdAt == requested.createdAt
        && current.expiresAt == requested.expiresAt
        && timerIntentSemanticIdentity(current.spec)
            == timerIntentSemanticIdentity(requested.spec);
}
}

TimerIntentApplicationService::TimerIntentApplicationService(
    TimerIntentRepository& intentRepository,
    TimerAssignmentRepository& assignmentRepository,
    NativeTimerBindingRepository& bindingRepository)
    : intentRepository_(intentRepository),
      assignmentRepository_(assignmentRepository),
      scheduling_(intentRepository, assignmentRepository),
      fulfillment_(assignmentRepository, bindingRepository)
{
}

TimerIntentApplicationResult
TimerIntentApplicationService::submitAndProvisionPrimary(
    const TimerIntentApplicationRequest& request)
{
    if (!requestValid(request))
        return result(TimerIntentApplicationStatus::invalid);

    TimerIntent intent;
    const auto created = intentRepository_.create(request.intent);
    if (created.ok())
    {
        intent = created.intent;
    }
    else if (created.status == TimerIntentRepositoryStatus::alreadyExists)
    {
        const auto found =
            intentRepository_.findById(request.intent.timerIntentId);
        if (!found.ok())
            return result(TimerIntentApplicationStatus::repositoryError);
        if (!sameIntentRequest(found.intent, request.intent))
            return result(
                TimerIntentApplicationStatus::intentConflict,
                found.intent);
        intent = found.intent;
    }
    else
    {
        return result(
            created.status == TimerIntentRepositoryStatus::invalid
                ? TimerIntentApplicationStatus::invalid
                : TimerIntentApplicationStatus::repositoryError);
    }

    if (intent.state == TimerIntentState::draft)
    {
        TimerIntent active = intent;
        active.state = TimerIntentState::active;
        active.updatedAt = request.activatedAt;
        const auto activated =
            intentRepository_.update(active, intent.intentRevision);
        if (!activated.ok())
            return result(
                activated.status == TimerIntentRepositoryStatus::conflict
                    ? TimerIntentApplicationStatus::intentConflict
                    : TimerIntentApplicationStatus::repositoryError,
                activated.intent);
        intent = activated.intent;
    }
    else if (intent.state != TimerIntentState::active)
    {
        return result(TimerIntentApplicationStatus::intentConflict, intent);
    }

    TimerAssignment assignment;
    const auto existingAssignment =
        assignmentRepository_.findById(request.timerAssignmentId);
    if (existingAssignment.ok())
    {
        assignment = existingAssignment.assignment;
    }
    else if (existingAssignment.status == TimerAssignmentRepositoryStatus::notFound)
    {
        TimerAssignmentPrimarySchedulingRequest schedulingRequest;
        schedulingRequest.timerAssignmentId = request.timerAssignmentId;
        schedulingRequest.timerIntentId = intent.timerIntentId;
        schedulingRequest.expectedIntentRevision = intent.intentRevision;
        schedulingRequest.createdAt = request.scheduledAt;
        schedulingRequest.candidates = request.candidates;
        const auto scheduled = scheduling_.schedulePrimary(schedulingRequest);

        if (scheduled.status ==
                TimerAssignmentSchedulingStatus::activePrimaryExists
            && scheduled.assignment.timerAssignmentId !=
                request.timerAssignmentId)
            return result(
                TimerIntentApplicationStatus::assignmentConflict,
                intent,
                scheduled.assignment);
        if (!scheduled.ok())
            return result(
                scheduled.status == TimerAssignmentSchedulingStatus::storageError
                    ? TimerIntentApplicationStatus::repositoryError
                    : TimerIntentApplicationStatus::schedulingConflict,
                intent,
                scheduled.assignment);
        assignment = scheduled.assignment;
    }
    else
    {
        return result(TimerIntentApplicationStatus::repositoryError, intent);
    }

    if (assignment.timerAssignmentId != request.timerAssignmentId
        || assignment.timerIntentId != intent.timerIntentId
        || assignment.intentRevision != intent.intentRevision
        || assignment.role != TimerAssignmentRole::primary)
        return result(
            TimerIntentApplicationStatus::assignmentConflict,
            intent,
            assignment);
    if (assignment.state == TimerAssignmentState::unassigned)
        return result(
            TimerIntentApplicationStatus::noEligibleBackend,
            intent,
            assignment);
    if (assignment.state == TimerAssignmentState::bound)
        return result(
            TimerIntentApplicationStatus::alreadyBound,
            intent,
            assignment);

    const auto provisioned = fulfillment_.beginProvisioning(
        assignment.timerAssignmentId,
        assignment.assignmentRevision,
        intent.intentRevision,
        assignment.backendGeneration,
        request.provisioningAt);
    switch (provisioned.status)
    {
        case TimerAssignmentFulfillmentStatus::provisioningStarted:
            return result(
                TimerIntentApplicationStatus::provisioningStarted,
                intent,
                provisioned.assignment);
        case TimerAssignmentFulfillmentStatus::alreadyProvisioning:
            return result(
                TimerIntentApplicationStatus::alreadyProvisioning,
                intent,
                provisioned.assignment);
        case TimerAssignmentFulfillmentStatus::repositoryError:
            return result(
                TimerIntentApplicationStatus::repositoryError,
                intent,
                provisioned.assignment);
        default:
            return result(
                TimerIntentApplicationStatus::provisioningConflict,
                intent,
                provisioned.assignment);
    }
}

}
