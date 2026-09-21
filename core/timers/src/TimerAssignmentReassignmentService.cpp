#include "TimerAssignmentReassignmentService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "TimerIntentRepository.h"

#include <cstddef>
#include <string>
#include <vector>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxReasonLength = 256;

bool safeIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

TimerAssignmentReassignmentResult result(
    TimerAssignmentReassignmentStatus status)
{
    TimerAssignmentReassignmentResult value;
    value.status = status;
    return value;
}

bool validRequest(const TimerAssignmentReassignmentRequest& request)
{
    if (!safeIdentity(request.replacementTimerAssignmentId)
        || !safeIdentity(request.oldTimerAssignmentId)
        || !safeIdentity(request.expectedOldAssignmentRevision)
        || request.expectedOldAssignmentEpoch == 0
        || !safeIdentity(request.expectedIntentRevision)
        || !safeIdentity(request.expectedOldBackendId)
        || request.expectedOldBackendGeneration == 0
        || request.reason.empty()
        || request.reason.size() > kMaxReasonLength
        || request.createdAt <= 0)
    {
        return false;
    }

    if (request.oldNativeOutcome
        == TimerAssignmentReassignmentNativeOutcome::beforeDispatch)
    {
        return request.oldOperationId.empty()
            && request.expectedOldOperationRevision.empty()
            && request.oldNativeTimerBindingId.empty()
            && request.expectedOldBindingRevision.empty();
    }

    return safeIdentity(request.oldOperationId)
        && safeIdentity(request.expectedOldOperationRevision)
        && safeIdentity(request.oldNativeTimerBindingId)
        && safeIdentity(request.expectedOldBindingRevision);
}

bool replayMatches(
    const TimerAssignmentControlledReplacementResult& existing,
    const TimerAssignmentReassignmentRequest& request)
{
    const auto& evidence = existing.evidence;
    return existing.replacementAssignment.timerIntentId
            == existing.oldAssignment.timerIntentId
        && existing.replacementAssignment.intentRevision
            == request.expectedIntentRevision
        && existing.replacementAssignment.createdAt == request.createdAt
        && evidence.replacementTimerAssignmentId
            == request.replacementTimerAssignmentId
        && evidence.oldTimerAssignmentId == request.oldTimerAssignmentId
        && evidence.oldAssignmentRevision
            == request.expectedOldAssignmentRevision
        && evidence.oldAssignmentEpoch == request.expectedOldAssignmentEpoch
        && evidence.oldBackendId == request.expectedOldBackendId
        && evidence.oldBackendGeneration
            == request.expectedOldBackendGeneration
        && evidence.oldNativeOutcome == request.oldNativeOutcome
        && evidence.oldOperationId == request.oldOperationId
        && evidence.oldOperationRevision
            == request.expectedOldOperationRevision
        && evidence.oldNativeTimerBindingId
            == request.oldNativeTimerBindingId
        && evidence.oldBindingRevision == request.expectedOldBindingRevision
        && evidence.reason == request.reason;
}

std::vector<TimerAssignment> activeAssignments(
    const std::vector<TimerAssignment>& assignments)
{
    std::vector<TimerAssignment> active;
    for (const auto& assignment : assignments)
    {
        if (timerAssignmentActiveOwnershipState(assignment.state))
            active.push_back(assignment);
    }
    return active;
}

TimerAssignment replacementFromDecision(
    const TimerAssignmentReassignmentRequest& request,
    const TimerAssignmentPlanningDecision& decision)
{
    TimerAssignment replacement;
    replacement.timerAssignmentId = request.replacementTimerAssignmentId;
    replacement.timerIntentId = decision.timerIntentId;
    replacement.intentRevision = decision.intentRevision;
    replacement.backendId = decision.selectedBackendId;
    replacement.backendGeneration = decision.selectedBackendGeneration;
    replacement.state = TimerAssignmentState::selected;
    replacement.role = TimerAssignmentRole::replacement;
    replacement.channelBinding = decision.selectedChannelBinding;
    replacement.capabilityRevision = decision.selectedCapabilityRevision;
    replacement.backendHealthRevision = decision.selectedBackendHealthRevision;
    replacement.decisionPolicyVersion = decision.decisionPolicyVersion;
    replacement.decisionEvidence = decision.decisionEvidence;
    replacement.createdAt = request.createdAt;
    replacement.updatedAt = request.createdAt;
    return replacement;
}

TimerAssignmentReassignmentEvidence evidenceFor(
    const TimerAssignmentReassignmentRequest& request,
    const TimerAssignment& replacement)
{
    TimerAssignmentReassignmentEvidence evidence;
    evidence.oldTimerAssignmentId = request.oldTimerAssignmentId;
    evidence.oldAssignmentRevision = request.expectedOldAssignmentRevision;
    evidence.oldAssignmentEpoch = request.expectedOldAssignmentEpoch;
    evidence.oldBackendId = request.expectedOldBackendId;
    evidence.oldBackendGeneration = request.expectedOldBackendGeneration;
    evidence.oldNativeOutcome = request.oldNativeOutcome;
    evidence.oldOperationId = request.oldOperationId;
    evidence.oldOperationRevision = request.expectedOldOperationRevision;
    evidence.oldNativeTimerBindingId = request.oldNativeTimerBindingId;
    evidence.oldBindingRevision = request.expectedOldBindingRevision;
    evidence.reason = request.reason;
    evidence.replacementTimerAssignmentId = request.replacementTimerAssignmentId;
    evidence.newBackendId = replacement.backendId;
    evidence.newBackendGeneration = replacement.backendGeneration;
    evidence.createdAt = request.createdAt;
    return evidence;
}

bool managedOwnership(NativeTimerBindingOwnership ownership)
{
    return ownership == NativeTimerBindingOwnership::managed
        || ownership == NativeTimerBindingOwnership::adopted;
}
} // namespace

TimerAssignmentReassignmentService::TimerAssignmentReassignmentService(
    TimerIntentRepository& intentRepository,
    TimerAssignmentRepository& assignmentRepository,
    NativeTimerBindingRepository& bindingRepository,
    vdrsuite::operations::MutationOperationRepository& operationRepository)
    : intentRepository_(intentRepository),
      assignmentRepository_(assignmentRepository),
      bindingRepository_(bindingRepository),
      operationRepository_(operationRepository)
{
}

TimerAssignmentReassignmentResult TimerAssignmentReassignmentService::reassign(
    const TimerAssignmentReassignmentRequest& request)
{
    using namespace vdrsuite::operations;
    if (!validRequest(request))
        return result(TimerAssignmentReassignmentStatus::invalidRequest);

    const auto replay = assignmentRepository_.findControlledReplacement(
        request.replacementTimerAssignmentId);
    if (replay.ok())
    {
        if (!replayMatches(replay, request))
            return result(TimerAssignmentReassignmentStatus::replacementIdConflict);
        TimerAssignmentReassignmentResult value;
        value.status = TimerAssignmentReassignmentStatus::alreadyPersisted;
        value.oldAssignment = replay.oldAssignment;
        value.replacementAssignment = replay.replacementAssignment;
        value.evidence = replay.evidence;
        return value;
    }
    if (replay.status != TimerAssignmentRepositoryStatus::notFound)
        return result(TimerAssignmentReassignmentStatus::repositoryError);

    const auto oldFound = assignmentRepository_.findById(request.oldTimerAssignmentId);
    if (oldFound.status == TimerAssignmentRepositoryStatus::notFound)
        return result(TimerAssignmentReassignmentStatus::assignmentNotFound);
    if (!oldFound.ok())
        return result(TimerAssignmentReassignmentStatus::repositoryError);
    const TimerAssignment& old = oldFound.assignment;

    const auto intentFound = intentRepository_.findById(old.timerIntentId);
    if (intentFound.status == TimerIntentRepositoryStatus::notFound)
        return result(TimerAssignmentReassignmentStatus::intentNotFound);
    if (!intentFound.ok())
        return result(TimerAssignmentReassignmentStatus::repositoryError);
    if (intentFound.intent.intentRevision != request.expectedIntentRevision
        || old.intentRevision != request.expectedIntentRevision)
        return result(TimerAssignmentReassignmentStatus::intentRevisionConflict);
    if (old.assignmentRevision != request.expectedOldAssignmentRevision)
        return result(TimerAssignmentReassignmentStatus::assignmentRevisionConflict);
    if (old.assignmentEpoch != request.expectedOldAssignmentEpoch)
        return result(TimerAssignmentReassignmentStatus::assignmentEpochConflict);
    if (old.backendId != request.expectedOldBackendId)
        return result(TimerAssignmentReassignmentStatus::backendConflict);
    if (old.backendGeneration != request.expectedOldBackendGeneration)
        return result(TimerAssignmentReassignmentStatus::generationConflict);
    if (old.role != TimerAssignmentRole::primary
        && old.role != TimerAssignmentRole::replacement)
        return result(TimerAssignmentReassignmentStatus::assignmentStateConflict);

    if (request.oldNativeOutcome
        == TimerAssignmentReassignmentNativeOutcome::beforeDispatch)
    {
        if (old.state != TimerAssignmentState::selected
            || !old.nativeTimerBindingId.empty())
            return result(TimerAssignmentReassignmentStatus::nativeOutcomeUnsafe);
    }
    else
    {
        if ((old.state != TimerAssignmentState::bound
             && old.state != TimerAssignmentState::reconciling)
            || old.nativeTimerBindingId != request.oldNativeTimerBindingId)
            return result(TimerAssignmentReassignmentStatus::nativeOutcomeUnsafe);

        const auto binding = bindingRepository_.findById(
            request.oldNativeTimerBindingId);
        if (binding.status == NativeTimerBindingRepositoryStatus::notFound)
            return result(TimerAssignmentReassignmentStatus::bindingNotFound);
        if (!binding.ok())
            return result(TimerAssignmentReassignmentStatus::repositoryError);
        if (binding.binding.bindingRevision != request.expectedOldBindingRevision
            || binding.binding.timerAssignmentId != old.timerAssignmentId
            || binding.binding.backendId != old.backendId
            || binding.binding.backendGeneration != old.backendGeneration)
            return result(TimerAssignmentReassignmentStatus::bindingConflict);
        if (!managedOwnership(binding.binding.ownership)
            || binding.binding.missingSince == 0
            || binding.binding.driftState
                != NativeTimerBindingDriftState::expectedTransition
            || binding.binding.observedState.recording
            || binding.binding.lastVerifiedOperationId != request.oldOperationId)
            return result(TimerAssignmentReassignmentStatus::nativeOutcomeUnsafe);

        const auto operation = operationRepository_.findById(request.oldOperationId);
        if (operation.status == MutationOperationRepositoryStatus::notFound)
            return result(TimerAssignmentReassignmentStatus::operationNotFound);
        if (!operation.ok())
            return result(TimerAssignmentReassignmentStatus::repositoryError);
        if (operation.operation.operationRevision
                != request.expectedOldOperationRevision
            || operation.operation.state != MutationOperationState::succeeded
            || operation.operation.actionFamily != "timer.delete"
            || operation.operation.resourceType != "NativeTimerBinding"
            || operation.operation.resourceId != binding.binding.nativeTimerBindingId
            || operation.operation.backendId != old.backendId
            || operation.operation.backendGeneration != old.backendGeneration)
            return result(TimerAssignmentReassignmentStatus::operationConflict);
    }

    const auto setRevision = assignmentRepository_.assignmentSetRevisionForIntent(
        old.timerIntentId);
    if (!setRevision.ok())
        return result(TimerAssignmentReassignmentStatus::repositoryError);
    const auto listed = assignmentRepository_.listForIntent(old.timerIntentId);
    if (!listed.ok())
        return result(TimerAssignmentReassignmentStatus::repositoryError);

    TimerAssignmentPlanningRequest planning;
    planning.intent = intentFound.intent;
    planning.role = TimerAssignmentRole::replacement;
    planning.currentAssignments = activeAssignments(listed.assignments);
    planning.candidates = request.candidates;

    TimerAssignmentReassignmentResult value;
    value.decision = planTimerAssignment(planning);
    if (value.decision.outcome == TimerAssignmentPlanningOutcome::invalid)
    {
        value.status = TimerAssignmentReassignmentStatus::planningInvalid;
        return value;
    }
    if (value.decision.outcome != TimerAssignmentPlanningOutcome::selected)
    {
        value.status = TimerAssignmentReassignmentStatus::noEligibleBackend;
        return value;
    }

    const TimerAssignment replacement = replacementFromDecision(request, value.decision);
    const auto persisted = assignmentRepository_.createControlledReplacement(
        replacement,
        setRevision.assignmentSetRevision,
        evidenceFor(request, replacement));
    if (!persisted.ok())
    {
        value.status = persisted.status == TimerAssignmentRepositoryStatus::conflict
            ? TimerAssignmentReassignmentStatus::assignmentSetConflict
            : persisted.status == TimerAssignmentRepositoryStatus::ownershipConflict
                ? TimerAssignmentReassignmentStatus::ownershipConflict
                : persisted.status == TimerAssignmentRepositoryStatus::alreadyExists
                    ? TimerAssignmentReassignmentStatus::replacementIdConflict
                    : TimerAssignmentReassignmentStatus::repositoryError;
        return value;
    }

    value.status = TimerAssignmentReassignmentStatus::persisted;
    value.oldAssignment = persisted.oldAssignment;
    value.replacementAssignment = persisted.replacementAssignment;
    value.evidence = persisted.evidence;
    return value;
}

const char* timerAssignmentReassignmentStatusName(
    TimerAssignmentReassignmentStatus status)
{
    switch (status)
    {
        case TimerAssignmentReassignmentStatus::persisted: return "persisted";
        case TimerAssignmentReassignmentStatus::alreadyPersisted: return "already_persisted";
        case TimerAssignmentReassignmentStatus::invalidRequest: return "invalid_request";
        case TimerAssignmentReassignmentStatus::replacementIdConflict: return "replacement_id_conflict";
        case TimerAssignmentReassignmentStatus::intentNotFound: return "intent_not_found";
        case TimerAssignmentReassignmentStatus::intentRevisionConflict: return "intent_revision_conflict";
        case TimerAssignmentReassignmentStatus::assignmentNotFound: return "assignment_not_found";
        case TimerAssignmentReassignmentStatus::assignmentRevisionConflict: return "assignment_revision_conflict";
        case TimerAssignmentReassignmentStatus::assignmentEpochConflict: return "assignment_epoch_conflict";
        case TimerAssignmentReassignmentStatus::assignmentStateConflict: return "assignment_state_conflict";
        case TimerAssignmentReassignmentStatus::backendConflict: return "backend_conflict";
        case TimerAssignmentReassignmentStatus::generationConflict: return "generation_conflict";
        case TimerAssignmentReassignmentStatus::assignmentSetConflict: return "assignment_set_conflict";
        case TimerAssignmentReassignmentStatus::nativeOutcomeUnsafe: return "native_outcome_unsafe";
        case TimerAssignmentReassignmentStatus::bindingNotFound: return "binding_not_found";
        case TimerAssignmentReassignmentStatus::bindingConflict: return "binding_conflict";
        case TimerAssignmentReassignmentStatus::operationNotFound: return "operation_not_found";
        case TimerAssignmentReassignmentStatus::operationConflict: return "operation_conflict";
        case TimerAssignmentReassignmentStatus::planningInvalid: return "planning_invalid";
        case TimerAssignmentReassignmentStatus::noEligibleBackend: return "no_eligible_backend";
        case TimerAssignmentReassignmentStatus::ownershipConflict: return "ownership_conflict";
        case TimerAssignmentReassignmentStatus::repositoryError: return "repository_error";
    }
    return "repository_error";
}

} // namespace vdrsuite::timers
