#include "TimerAssignment.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxEvidenceTextLength = 256;
constexpr std::size_t kMaxEvidenceEntries = 32;
constexpr std::int32_t kMaxDecisionScoreMagnitude = 1000000;

bool bounded(const std::string& value, std::size_t maximum)
{
    return value.size() <= maximum;
}

bool nonEmptyBounded(const std::string& value, std::size_t maximum)
{
    return !value.empty() && bounded(value, maximum);
}

bool boundedEvidence(const std::vector<std::string>& values)
{
    if (values.size() > kMaxEvidenceEntries) return false;
    return std::all_of(
        values.begin(),
        values.end(),
        [](const std::string& value)
        {
            return nonEmptyBounded(value, kMaxEvidenceTextLength);
        });
}

bool decisionEvidenceValid(const TimerAssignmentDecisionEvidence& evidence)
{
    return !evidence.reasons.empty()
        && boundedEvidence(evidence.reasons)
        && boundedEvidence(evidence.warnings)
        && boundedEvidence(evidence.exclusions)
        && boundedEvidence(evidence.conflictFacts)
        && evidence.decisionScore >= -kMaxDecisionScoreMagnitude
        && evidence.decisionScore <= kMaxDecisionScoreMagnitude;
}

bool channelBindingPresent(const TimerAssignmentChannelBinding& binding)
{
    return !binding.canonicalChannelId.empty()
        || !binding.backendChannelId.empty()
        || !binding.mappingSource.empty()
        || !binding.mappingRevision.empty();
}

bool channelBindingValid(const TimerAssignmentChannelBinding& binding)
{
    if (!channelBindingPresent(binding)) return false;
    return bounded(binding.canonicalChannelId, kMaxIdentityLength)
        && nonEmptyBounded(binding.backendChannelId, kMaxIdentityLength)
        && nonEmptyBounded(binding.mappingSource, kMaxIdentityLength)
        && nonEmptyBounded(binding.mappingRevision, kMaxIdentityLength);
}

bool unassignedShapeValid(const TimerAssignment& assignment)
{
    return assignment.backendId.empty()
        && assignment.backendGeneration == 0
        && !channelBindingPresent(assignment.channelBinding)
        && assignment.capabilityRevision.empty()
        && assignment.backendHealthRevision.empty()
        && assignment.nativeTimerBindingId.empty();
}

bool assignedShapeValid(const TimerAssignment& assignment)
{
    return nonEmptyBounded(assignment.backendId, kMaxIdentityLength)
        && assignment.backendGeneration > 0
        && channelBindingValid(assignment.channelBinding)
        && nonEmptyBounded(assignment.capabilityRevision, kMaxIdentityLength)
        && nonEmptyBounded(assignment.backendHealthRevision, kMaxIdentityLength);
}
}

const char* timerAssignmentStateName(TimerAssignmentState state)
{
    switch (state)
    {
        case TimerAssignmentState::proposed: return "proposed";
        case TimerAssignmentState::selected: return "selected";
        case TimerAssignmentState::provisioning: return "provisioning";
        case TimerAssignmentState::bound: return "bound";
        case TimerAssignmentState::reconciling: return "reconciling";
        case TimerAssignmentState::unassigned: return "unassigned";
        case TimerAssignmentState::superseding: return "superseding";
        case TimerAssignmentState::superseded: return "superseded";
        case TimerAssignmentState::cancelRequested: return "cancel_requested";
        case TimerAssignmentState::cancelled: return "cancelled";
        case TimerAssignmentState::failed: return "failed";
    }
    return "unknown";
}

const char* timerAssignmentRoleName(TimerAssignmentRole role)
{
    switch (role)
    {
        case TimerAssignmentRole::primary: return "primary";
        case TimerAssignmentRole::replica: return "replica";
        case TimerAssignmentRole::replacement: return "replacement";
    }
    return "unknown";
}

bool timerAssignmentValid(const TimerAssignment& assignment)
{
    if (!nonEmptyBounded(assignment.timerAssignmentId, kMaxIdentityLength)
        || !nonEmptyBounded(assignment.assignmentRevision, kMaxIdentityLength)
        || !nonEmptyBounded(assignment.timerIntentId, kMaxIdentityLength)
        || !nonEmptyBounded(assignment.intentRevision, kMaxIdentityLength)
        || assignment.assignmentEpoch == 0
        || !nonEmptyBounded(assignment.decisionPolicyVersion, kMaxIdentityLength)
        || !decisionEvidenceValid(assignment.decisionEvidence)
        || !bounded(assignment.nativeTimerBindingId, kMaxIdentityLength)
        || assignment.createdAt <= 0
        || assignment.updatedAt < assignment.createdAt)
    {
        return false;
    }

    if (assignment.state == TimerAssignmentState::unassigned)
    {
        return unassignedShapeValid(assignment);
    }

    if (!assignedShapeValid(assignment)) return false;

    if (assignment.state == TimerAssignmentState::bound)
    {
        return nonEmptyBounded(
            assignment.nativeTimerBindingId,
            kMaxIdentityLength);
    }

    return true;
}

bool timerAssignmentRevisionMatches(
    const std::string& expectedRevision,
    const std::string& currentRevision)
{
    return !expectedRevision.empty()
        && !currentRevision.empty()
        && expectedRevision == currentRevision;
}

bool timerAssignmentCanTransition(
    TimerAssignmentState from,
    TimerAssignmentState to)
{
    if (from == to) return false;

    switch (from)
    {
        case TimerAssignmentState::proposed:
            return to == TimerAssignmentState::selected
                || to == TimerAssignmentState::unassigned
                || to == TimerAssignmentState::cancelRequested
                || to == TimerAssignmentState::failed;
        case TimerAssignmentState::selected:
            return to == TimerAssignmentState::provisioning
                || to == TimerAssignmentState::unassigned
                || to == TimerAssignmentState::superseding
                || to == TimerAssignmentState::cancelRequested
                || to == TimerAssignmentState::failed;
        case TimerAssignmentState::provisioning:
            return to == TimerAssignmentState::bound
                || to == TimerAssignmentState::reconciling
                || to == TimerAssignmentState::unassigned
                || to == TimerAssignmentState::superseding
                || to == TimerAssignmentState::cancelRequested
                || to == TimerAssignmentState::failed;
        case TimerAssignmentState::bound:
            return to == TimerAssignmentState::reconciling
                || to == TimerAssignmentState::superseding
                || to == TimerAssignmentState::cancelRequested
                || to == TimerAssignmentState::failed;
        case TimerAssignmentState::reconciling:
            return to == TimerAssignmentState::bound
                || to == TimerAssignmentState::unassigned
                || to == TimerAssignmentState::superseding
                || to == TimerAssignmentState::cancelRequested
                || to == TimerAssignmentState::failed;
        case TimerAssignmentState::unassigned:
            return to == TimerAssignmentState::proposed
                || to == TimerAssignmentState::selected
                || to == TimerAssignmentState::cancelRequested
                || to == TimerAssignmentState::failed;
        case TimerAssignmentState::superseding:
            return to == TimerAssignmentState::superseded
                || to == TimerAssignmentState::reconciling
                || to == TimerAssignmentState::cancelRequested
                || to == TimerAssignmentState::failed;
        case TimerAssignmentState::cancelRequested:
            return to == TimerAssignmentState::cancelled
                || to == TimerAssignmentState::failed;
        case TimerAssignmentState::superseded:
        case TimerAssignmentState::cancelled:
        case TimerAssignmentState::failed:
            return false;
    }
    return false;
}

bool timerAssignmentActiveOwnershipState(TimerAssignmentState state)
{
    return state == TimerAssignmentState::selected
        || state == TimerAssignmentState::provisioning
        || state == TimerAssignmentState::bound
        || state == TimerAssignmentState::reconciling
        || state == TimerAssignmentState::superseding;
}

bool timerAssignmentTerminal(TimerAssignmentState state)
{
    return state == TimerAssignmentState::superseded
        || state == TimerAssignmentState::cancelled
        || state == TimerAssignmentState::failed;
}

}
