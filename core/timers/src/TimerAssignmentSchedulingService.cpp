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

bool validSchedulingRequest(
    const std::string& timerAssignmentId,
    const std::string& timerIntentId,
    const std::string& expectedIntentRevision,
    std::int64_t createdAt)
{
    return safeIdentity(timerAssignmentId)
        && safeIdentity(timerIntentId)
        && safeIdentity(expectedIntentRevision)
        && createdAt > 0;
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
    const std::string& timerAssignmentId,
    const std::string& timerIntentId,
    const std::string& expectedIntentRevision,
    TimerAssignmentRole role,
    std::int64_t createdAt)
{
    return assignment.timerAssignmentId == timerAssignmentId
        && assignment.timerIntentId == timerIntentId
        && assignment.intentRevision == expectedIntentRevision
        && assignment.role == role
        && assignment.createdAt == createdAt
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
    const std::string& timerAssignmentId,
    std::int64_t createdAt,
    TimerAssignmentRole role,
    const TimerAssignmentPlanningDecision& decision)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = timerAssignmentId;
    assignment.timerIntentId = decision.timerIntentId;
    assignment.intentRevision = decision.intentRevision;
    assignment.role = role;
    assignment.createdAt = createdAt;
    assignment.updatedAt = createdAt;
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

TimerAssignmentSchedulingResult existingAssignmentResult(
    TimerAssignmentRepository& repository,
    const std::string& timerAssignmentId,
    const std::string& timerIntentId,
    const std::string& expectedIntentRevision,
    TimerAssignmentRole role,
    std::int64_t createdAt,
    bool& handled)
{
    handled = true;
    const auto existing = repository.findById(timerAssignmentId);
    if (existing.ok())
    {
        return idempotentAssignmentMatches(
                   existing.assignment,
                   timerAssignmentId,
                   timerIntentId,
                   expectedIntentRevision,
                   role,
                   createdAt)
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

    handled = false;
    return statusResult(TimerAssignmentSchedulingStatus::storageError);
}

void applyCreateResult(
    TimerAssignmentSchedulingResult& result,
    TimerAssignmentRepository& repository,
    const TimerAssignmentRepositoryResult& created,
    const std::string& timerAssignmentId,
    const std::string& timerIntentId,
    const std::string& expectedIntentRevision,
    TimerAssignmentRole role,
    std::int64_t createdAt,
    bool assignmentSetFenced)
{
    if (created.ok())
    {
        result.status = TimerAssignmentSchedulingStatus::persisted;
        result.assignment = created.assignment;
        return;
    }

    if (created.status == TimerAssignmentRepositoryStatus::alreadyExists)
    {
        const auto retry = repository.findById(timerAssignmentId);
        if (retry.ok()
            && idempotentAssignmentMatches(
                retry.assignment,
                timerAssignmentId,
                timerIntentId,
                expectedIntentRevision,
                role,
                createdAt))
        {
            result.status =
                TimerAssignmentSchedulingStatus::alreadyPersisted;
            result.assignment = retry.assignment;
            return;
        }
        result.status =
            TimerAssignmentSchedulingStatus::assignmentIdConflict;
        if (retry.ok()) result.assignment = retry.assignment;
        return;
    }

    if (assignmentSetFenced
        && created.status == TimerAssignmentRepositoryStatus::conflict)
    {
        result.status = TimerAssignmentSchedulingStatus::assignmentSetConflict;
        result.assignment = created.assignment;
        return;
    }

    result.status = mapRepositoryStatus(created.status);
    result.assignment = created.assignment;
}

} // namespace

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
    if (!validSchedulingRequest(
            request.timerAssignmentId,
            request.timerIntentId,
            request.expectedIntentRevision,
            request.createdAt))
    {
        return statusResult(
            TimerAssignmentSchedulingStatus::invalidRequest);
    }

    bool existingHandled = false;
    TimerAssignmentSchedulingResult existingResult =
        existingAssignmentResult(
            assignmentRepository_,
            request.timerAssignmentId,
            request.timerIntentId,
            request.expectedIntentRevision,
            TimerAssignmentRole::primary,
            request.createdAt,
            existingHandled);
    if (existingHandled) return existingResult;

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
        assignmentFromDecision(
            request.timerAssignmentId,
            request.createdAt,
            TimerAssignmentRole::primary,
            result.decision);
    const auto created = assignmentRepository_.create(candidate);
    applyCreateResult(
        result,
        assignmentRepository_,
        created,
        request.timerAssignmentId,
        request.timerIntentId,
        request.expectedIntentRevision,
        TimerAssignmentRole::primary,
        request.createdAt,
        false);
    return result;
}

TimerAssignmentSchedulingResult
TimerAssignmentSchedulingService::scheduleReplica(
    const TimerAssignmentReplicaSchedulingRequest& request)
{
    if (!validSchedulingRequest(
            request.timerAssignmentId,
            request.timerIntentId,
            request.expectedIntentRevision,
            request.createdAt))
    {
        return statusResult(
            TimerAssignmentSchedulingStatus::invalidRequest);
    }

    bool existingHandled = false;
    TimerAssignmentSchedulingResult existingResult =
        existingAssignmentResult(
            assignmentRepository_,
            request.timerAssignmentId,
            request.timerIntentId,
            request.expectedIntentRevision,
            TimerAssignmentRole::replica,
            request.createdAt,
            existingHandled);
    if (existingHandled) return existingResult;

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

    // The set revision is intentionally read before the assignment list. Any
    // assignment mutation after this point makes the final fenced create stale.
    const auto setRevision =
        assignmentRepository_.assignmentSetRevisionForIntent(
            request.timerIntentId);
    if (!setRevision.ok())
    {
        return statusResult(mapRepositoryStatus(setRevision.status));
    }

    const auto listed =
        assignmentRepository_.listForIntent(request.timerIntentId);
    if (!listed.ok())
    {
        return statusResult(mapRepositoryStatus(listed.status));
    }

    TimerAssignmentPlanningRequest planning;
    planning.intent = intent.intent;
    planning.role = TimerAssignmentRole::replica;
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
        && decisionHasReason(result.decision, "replica_target_satisfied"))
    {
        result.status =
            TimerAssignmentSchedulingStatus::replicaTargetSatisfied;
        return result;
    }

    const TimerAssignment candidate =
        assignmentFromDecision(
            request.timerAssignmentId,
            request.createdAt,
            TimerAssignmentRole::replica,
            result.decision);
    const auto created =
        assignmentRepository_.createAgainstAssignmentSetRevision(
            candidate,
            setRevision.assignmentSetRevision);

    applyCreateResult(
        result,
        assignmentRepository_,
        created,
        request.timerAssignmentId,
        request.timerIntentId,
        request.expectedIntentRevision,
        TimerAssignmentRole::replica,
        request.createdAt,
        true);
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
        case TimerAssignmentSchedulingStatus::replicaTargetSatisfied:
            return "replica_target_satisfied";
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
        case TimerAssignmentSchedulingStatus::assignmentSetConflict:
            return "assignment_set_conflict";
        case TimerAssignmentSchedulingStatus::repositoryConflict:
            return "repository_conflict";
        case TimerAssignmentSchedulingStatus::storageError:
            return "storage_error";
    }
    return "storage_error";
}

}
