#include "TimerAssignmentSchedulingService.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;

bool safeIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

TimerAssignmentSchedulingResult statusResult(
    TimerAssignmentSchedulingStatus status)
{
    TimerAssignmentSchedulingResult result;
    result.status = status;
    return result;
}

TimerAssignmentSchedulingResult statusResult(
    TimerAssignmentSchedulingStatus status,
    const TimerAssignment& assignment)
{
    TimerAssignmentSchedulingResult result;
    result.status = status;
    result.assignment = assignment;
    return result;
}

bool idempotentAssignmentMatches(
    const TimerAssignment& assignment,
    const TimerAssignmentPrimarySchedulingRequest& request)
{
    return assignment.timerAssignmentId == request.timerAssignmentId
        && assignment.timerIntentId == request.timerIntentId
        && assignment.intentRevision == request.expectedIntentRevision
        && assignment.role == TimerAssignmentRole::primary
        && assignment.createdAt == request.createdAt
        && (assignment.state == TimerAssignmentState::selected
            || assignment.state == TimerAssignmentState::unassigned)
        && assignment.decisionPolicyVersion
            == timerAssignmentPlanningPolicyVersion();
}

std::vector<TimerAssignment> activeAssignments(
    const std::vector<TimerAssignment>& assignments)
{
    std::vector<TimerAssignment> result;
    result.reserve(assignments.size());
    for (const auto& assignment : assignments)
    {
        if (timerAssignmentActiveOwnershipState(assignment.state))
        {
            result.push_back(assignment);
        }
    }
    return result;
}

bool decisionHasReason(
    const TimerAssignmentPlanningDecision& decision,
    const std::string& reason)
{
    return std::find(
        decision.decisionEvidence.reasons.begin(),
        decision.decisionEvidence.reasons.end(),
        reason) != decision.decisionEvidence.reasons.end();
}

TimerAssignment assignmentFromDecision(
    const TimerAssignmentPrimarySchedulingRequest& request,
    const TimerAssignmentPlanningDecision& decision)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = request.timerAssignmentId;
    assignment.timerIntentId = decision.timerIntentId;
    assignment.intentRevision = decision.intentRevision;
    assignment.role = TimerAssignmentRole::primary;
    assignment.createdAt = request.createdAt;
    assignment.updatedAt = request.createdAt;
    assignment.decisionPolicyVersion = decision.decisionPolicyVersion;
    assignment.decisionEvidence = decision.decisionEvidence;

    if (decision.outcome == TimerAssignmentPlanningOutcome::selected)
    {
        assignment.state = TimerAssignmentState::selected;
        assignment.backendId = decision.selectedBackendId;
        assignment.backendGeneration = decision.selectedBackendGeneration;
        assignment.channelBinding = decision.selectedChannelBinding;
        assignment.capabilityRevision =
            decision.selectedCapabilityRevision;
        assignment.backendHealthRevision =
            decision.selectedBackendHealthRevision;
    }
    else
    {
        assignment.state = TimerAssignmentState::unassigned;
    }

    return assignment;
}

TimerAssignmentSchedulingStatus mapRepositoryStatus(
    TimerAssignmentRepositoryStatus status)
{
    switch (status)
    {
        case TimerAssignmentRepositoryStatus::ownershipConflict:
            return TimerAssignmentSchedulingStatus::ownershipConflict;
        case TimerAssignmentRepositoryStatus::intentNotFound:
            return TimerAssignmentSchedulingStatus::intentNotFound;
        case TimerAssignmentRepositoryStatus::intentRevisionConflict:
            return TimerAssignmentSchedulingStatus::intentRevisionConflict;
        case TimerAssignmentRepositoryStatus::invalid:
            return TimerAssignmentSchedulingStatus::planningInvalid;
        case TimerAssignmentRepositoryStatus::conflict:
            return TimerAssignmentSchedulingStatus::repositoryConflict;
        case TimerAssignmentRepositoryStatus::storageError:
            return TimerAssignmentSchedulingStatus::storageError;
        case TimerAssignmentRepositoryStatus::alreadyExists:
            return TimerAssignmentSchedulingStatus::assignmentIdConflict;
        case TimerAssignmentRepositoryStatus::notFound:
            return TimerAssignmentSchedulingStatus::repositoryConflict;
        case TimerAssignmentRepositoryStatus::ok:
            return TimerAssignmentSchedulingStatus::persisted;
    }
    return TimerAssignmentSchedulingStatus::storageError;
}

TimerAssignmentSchedulingStatus mapIntentReadStatus(
    TimerIntentRepositoryStatus status)
{
    switch (status)
    {
        case TimerIntentRepositoryStatus::notFound:
            return TimerAssignmentSchedulingStatus::intentNotFound;
        case TimerIntentRepositoryStatus::invalid:
            return TimerAssignmentSchedulingStatus::invalidRequest;
        case TimerIntentRepositoryStatus::storageError:
            return TimerAssignmentSchedulingStatus::storageError;
        case TimerIntentRepositoryStatus::conflict:
        case TimerIntentRepositoryStatus::alreadyExists:
            return TimerAssignmentSchedulingStatus::repositoryConflict;
        case TimerIntentRepositoryStatus::ok:
            return TimerAssignmentSchedulingStatus::persisted;
    }
    return TimerAssignmentSchedulingStatus::storageError;
}
}

TimerAssignmentSchedulingService::TimerAssignmentSchedulingService(
    TimerIntentRepository& intentRepository,
    TimerAssignmentRepository& assignmentRepository)
    : intentRepository_(intentRepository),
      assignmentRepository_(assignmentRepository)
{
}

TimerAssignmentSchedulingResult
TimerAssignmentSchedulingService::schedulePrimary(
    const TimerAssignmentPrimarySchedulingRequest& request)
{
    if (!safeIdentity(request.timerAssignmentId)
        || !safeIdentity(request.timerIntentId)
        || !safeIdentity(request.expectedIntentRevision)
        || request.createdAt <= 0)
    {
        return statusResult(
            TimerAssignmentSchedulingStatus::invalidRequest);
    }

    const auto existing =
        assignmentRepository_.findById(request.timerAssignmentId);
    if (existing.ok())
    {
        return idempotentAssignmentMatches(existing.assignment, request)
            ? statusResult(
                TimerAssignmentSchedulingStatus::alreadyPersisted,
                existing.assignment)
            : statusResult(
                TimerAssignmentSchedulingStatus::assignmentIdConflict,
                existing.assignment);
    }
    if (existing.status != TimerAssignmentRepositoryStatus::notFound)
    {
        return statusResult(mapRepositoryStatus(existing.status));
    }

    const auto intent =
        intentRepository_.findById(request.timerIntentId);
    if (!intent.ok())
    {
        return statusResult(mapIntentReadStatus(intent.status));
    }
    if (intent.intent.intentRevision != request.expectedIntentRevision)
    {
        return statusResult(
            TimerAssignmentSchedulingStatus::intentRevisionConflict);
    }

    const auto listed =
        assignmentRepository_.listForIntent(request.timerIntentId);
    if (!listed.ok())
    {
        return statusResult(mapRepositoryStatus(listed.status));
    }

    TimerAssignmentPlanningRequest planning;
    planning.intent = intent.intent;
    planning.role = TimerAssignmentRole::primary;
    planning.currentAssignments = activeAssignments(listed.assignments);
    planning.candidates = request.candidates;

    TimerAssignmentSchedulingResult result;
    result.decision = planTimerAssignment(planning);

    if (result.decision.outcome == TimerAssignmentPlanningOutcome::invalid)
    {
        result.status = TimerAssignmentSchedulingStatus::planningInvalid;
        return result;
    }

    if (result.decision.outcome == TimerAssignmentPlanningOutcome::unassigned
        && decisionHasReason(result.decision, "active_primary_exists"))
    {
        const auto active =
            assignmentRepository_.findActivePrimaryForIntent(
                request.timerIntentId);
        if (active.ok())
        {
            result.status =
                TimerAssignmentSchedulingStatus::activePrimaryExists;
            result.assignment = active.assignment;
            return result;
        }

        result.status =
            active.status == TimerAssignmentRepositoryStatus::storageError
                ? TimerAssignmentSchedulingStatus::storageError
                : TimerAssignmentSchedulingStatus::repositoryConflict;
        return result;
    }

    const TimerAssignment candidate =
        assignmentFromDecision(request, result.decision);
    const auto created = assignmentRepository_.create(candidate);
    if (created.ok())
    {
        result.status = TimerAssignmentSchedulingStatus::persisted;
        result.assignment = created.assignment;
        return result;
    }

    if (created.status == TimerAssignmentRepositoryStatus::alreadyExists)
    {
        const auto retry =
            assignmentRepository_.findById(request.timerAssignmentId);
        if (retry.ok()
            && idempotentAssignmentMatches(retry.assignment, request))
        {
            result.status =
                TimerAssignmentSchedulingStatus::alreadyPersisted;
            result.assignment = retry.assignment;
            return result;
        }
        result.status =
            TimerAssignmentSchedulingStatus::assignmentIdConflict;
        if (retry.ok()) result.assignment = retry.assignment;
        return result;
    }

    result.status = mapRepositoryStatus(created.status);
    result.assignment = created.assignment;
    return result;
}

const char* timerAssignmentSchedulingStatusName(
    TimerAssignmentSchedulingStatus status)
{
    switch (status)
    {
        case TimerAssignmentSchedulingStatus::persisted:
            return "persisted";
        case TimerAssignmentSchedulingStatus::alreadyPersisted:
            return "already_persisted";
        case TimerAssignmentSchedulingStatus::activePrimaryExists:
            return "active_primary_exists";
        case TimerAssignmentSchedulingStatus::invalidRequest:
            return "invalid_request";
        case TimerAssignmentSchedulingStatus::assignmentIdConflict:
            return "assignment_id_conflict";
        case TimerAssignmentSchedulingStatus::intentNotFound:
            return "intent_not_found";
        case TimerAssignmentSchedulingStatus::intentRevisionConflict:
            return "intent_revision_conflict";
        case TimerAssignmentSchedulingStatus::planningInvalid:
            return "planning_invalid";
        case TimerAssignmentSchedulingStatus::ownershipConflict:
            return "ownership_conflict";
        case TimerAssignmentSchedulingStatus::repositoryConflict:
            return "repository_conflict";
        case TimerAssignmentSchedulingStatus::storageError:
            return "storage_error";
    }
    return "storage_error";
}

}
