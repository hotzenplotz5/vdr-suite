#include "TimerAssignmentPlanner.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxCandidates = 32;
constexpr std::size_t kMaxEvidenceEntries = 32;
constexpr std::size_t kMaxEvidenceTextLength = 256;
constexpr std::uint32_t kOrdinaryPreferenceRank = 32;

bool bounded(const std::string& value, std::size_t maximum)
{
    return value.size() <= maximum;
}

bool nonEmptyBounded(const std::string& value, std::size_t maximum)
{
    return !value.empty() && bounded(value, maximum);
}

bool contains(
    const std::vector<std::string>& values,
    const std::string& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

std::uint32_t preferenceRank(
    const TimerIntentAssignmentPolicy& policy,
    const std::string& backendId,
    bool& preferred)
{
    const auto it = std::find(
        policy.preferredBackendIds.begin(),
        policy.preferredBackendIds.end(),
        backendId);
    if (it == policy.preferredBackendIds.end())
    {
        preferred = false;
        return kOrdinaryPreferenceRank;
    }

    preferred = true;
    return static_cast<std::uint32_t>(
        std::distance(policy.preferredBackendIds.begin(), it));
}

void appendBounded(
    std::vector<std::string>& values,
    const std::string& value)
{
    if (values.size() >= kMaxEvidenceEntries
        || value.empty()
        || value.size() > kMaxEvidenceTextLength)
    {
        return;
    }
    values.push_back(value);
}

std::string backendFact(
    const std::string& backendId,
    const std::string& fact)
{
    return backendId + ":" + fact;
}

bool usableBackendState(TimerAssignmentPlanningBackendState state)
{
    return state == TimerAssignmentPlanningBackendState::online
        || state == TimerAssignmentPlanningBackendState::degraded;
}

bool usableHealthState(TimerAssignmentPlanningHealthState state)
{
    return state == TimerAssignmentPlanningHealthState::healthy
        || state == TimerAssignmentPlanningHealthState::degraded;
}

bool selectedTargetEmpty(const TimerAssignmentPlanningDecision& decision)
{
    return decision.selectedBackendId.empty()
        && decision.selectedBackendGeneration == 0
        && decision.selectedChannelBinding.canonicalChannelId.empty()
        && decision.selectedChannelBinding.backendChannelId.empty()
        && decision.selectedChannelBinding.mappingSource.empty()
        && decision.selectedChannelBinding.mappingRevision.empty()
        && decision.selectedCapabilityRevision.empty()
        && decision.selectedBackendHealthRevision.empty();
}

bool candidateIdentityValid(
    const TimerAssignmentPlanningBackendCandidate& candidate)
{
    return nonEmptyBounded(candidate.backendId, kMaxIdentityLength)
        && bounded(candidate.siteId, kMaxIdentityLength)
        && bounded(candidate.executionAuthorityFence, kMaxIdentityLength)
        && bounded(candidate.capability.revision, kMaxIdentityLength)
        && bounded(candidate.health.revision, kMaxIdentityLength)
        && bounded(candidate.channel.mappingRevision, kMaxIdentityLength)
        && bounded(candidate.channel.mappingSource, kMaxIdentityLength)
        && bounded(candidate.channel.canonicalChannelId, kMaxIdentityLength)
        && bounded(candidate.channel.backendChannelId, kMaxIdentityLength);
}

bool requestShapeValid(const TimerAssignmentPlanningRequest& request)
{
    if (!timerIntentValid(request.intent)
        || !timerIntentAssignable(request.intent.state)
        || request.candidates.size() > kMaxCandidates
        || request.currentAssignments.size() > kMaxCandidates)
    {
        return false;
    }

    if (request.role == TimerAssignmentRole::replica
        && (request.intent.spec.replicaPolicy.desiredAssignments <= 1
            || !request.intent.spec.replicaPolicy.simultaneousRecordingIntentional))
    {
        return false;
    }

    if (request.role == TimerAssignmentRole::replacement
        && !request.intent.spec.assignmentPolicy.allowFailover)
    {
        return false;
    }

    std::set<std::string> candidateIds;
    for (const auto& candidate : request.candidates)
    {
        if (!candidateIdentityValid(candidate)
            || !candidateIds.insert(candidate.backendId).second)
        {
            return false;
        }
    }

    for (const auto& assignment : request.currentAssignments)
    {
        if (!timerAssignmentValid(assignment)
            || assignment.timerIntentId != request.intent.timerIntentId)
        {
            return false;
        }
    }

    return true;
}

std::set<std::string> activeBackendIds(
    const std::vector<TimerAssignment>& assignments)
{
    std::set<std::string> result;
    for (const auto& assignment : assignments)
    {
        if (timerAssignmentActiveOwnershipState(assignment.state)
            && !assignment.backendId.empty())
        {
            result.insert(assignment.backendId);
        }
    }
    return result;
}

std::set<std::string> activeSiteIds(
    const std::vector<TimerAssignment>& assignments,
    const std::vector<TimerAssignmentPlanningBackendCandidate>& candidates,
    bool& complete)
{
    std::set<std::string> result;
    complete = true;

    for (const auto& assignment : assignments)
    {
        if (!timerAssignmentActiveOwnershipState(assignment.state)
            || assignment.backendId.empty())
        {
            continue;
        }

        const auto it = std::find_if(
            candidates.begin(),
            candidates.end(),
            [&](const TimerAssignmentPlanningBackendCandidate& candidate)
            {
                return candidate.backendId == assignment.backendId;
            });

        if (it == candidates.end() || it->siteId.empty())
        {
            complete = false;
            continue;
        }
        result.insert(it->siteId);
    }

    return result;
}

bool hasActivePrimary(const std::vector<TimerAssignment>& assignments)
{
    return std::any_of(
        assignments.begin(),
        assignments.end(),
        [](const TimerAssignment& assignment)
        {
            return assignment.role == TimerAssignmentRole::primary
                && timerAssignmentActiveOwnershipState(assignment.state);
        });
}

std::size_t activeReplicaOwnerCount(
    const std::vector<TimerAssignment>& assignments)
{
    return static_cast<std::size_t>(std::count_if(
        assignments.begin(),
        assignments.end(),
        [](const TimerAssignment& assignment)
        {
            return timerAssignmentActiveOwnershipState(assignment.state)
                && (assignment.role == TimerAssignmentRole::primary
                    || assignment.role == TimerAssignmentRole::replica);
        }));
}

TimerAssignmentPlanningCandidateEvaluation evaluateCandidate(
    const TimerAssignmentPlanningRequest& request,
    const TimerAssignmentPlanningBackendCandidate& candidate,
    const std::set<std::string>& usedBackends,
    const std::set<std::string>& usedSites,
    bool siteEvidenceComplete)
{
    TimerAssignmentPlanningCandidateEvaluation result;
    result.backendId = candidate.backendId;
    result.preferenceRank = preferenceRank(
        request.intent.spec.assignmentPolicy,
        candidate.backendId,
        result.preferred);

    const auto exclude = [&](const std::string& reason)
    {
        appendBounded(result.exclusions, reason);
    };

    if (contains(
            request.intent.spec.assignmentPolicy.excludedBackendIds,
            candidate.backendId))
    {
        exclude("excluded_by_intent");
    }

    if (!usableBackendState(candidate.state))
    {
        exclude(std::string("backend_")
            + timerAssignmentPlanningBackendStateName(candidate.state));
    }

    if (!candidate.writeAllowed)
    {
        exclude("backend_write_forbidden");
    }

    if (!candidate.executionAuthorityCurrent
        || candidate.executionAuthorityFence.empty())
    {
        exclude("execution_authority_unavailable");
    }

    if (candidate.currentBackendGeneration == 0)
    {
        exclude("backend_generation_missing");
    }

    if (!candidate.capability.current
        || candidate.capability.revision.empty())
    {
        exclude("capability_missing_or_stale");
    }
    else if (candidate.capability.backendGeneration
        != candidate.currentBackendGeneration)
    {
        exclude("capability_generation_stale");
    }
    else if (!candidate.capability.timerCreate
        || !candidate.capability.timerReadback)
    {
        exclude("timer_capability_missing");
    }

    if (!candidate.health.current || candidate.health.revision.empty())
    {
        exclude("health_missing_or_stale");
    }
    else if (candidate.health.backendGeneration
        != candidate.currentBackendGeneration)
    {
        exclude("health_generation_stale");
    }
    else if (!usableHealthState(candidate.health.state)
        || !candidate.health.timerWritesAvailable)
    {
        exclude("timer_health_unavailable");
    }
    else if (candidate.health.state
        == TimerAssignmentPlanningHealthState::degraded)
    {
        appendBounded(result.warnings, "backend_health_degraded");
    }

    if (!candidate.channel.current
        || candidate.channel.mappingRevision.empty()
        || candidate.channel.mappingSource.empty()
        || candidate.channel.backendChannelId.empty())
    {
        exclude("channel_mapping_missing_or_stale");
    }
    else if (candidate.channel.backendGeneration
        != candidate.currentBackendGeneration)
    {
        exclude("channel_mapping_generation_stale");
    }
    else if (candidate.channel.ambiguous)
    {
        exclude("channel_mapping_ambiguous");
    }
    else if (!request.intent.spec.channelRequirement.canonicalChannelId.empty()
        && candidate.channel.canonicalChannelId
            != request.intent.spec.channelRequirement.canonicalChannelId)
    {
        exclude("channel_mapping_mismatch");
    }

    switch (candidate.conflict)
    {
        case TimerAssignmentPlanningConflictState::confirmedClear:
            appendBounded(result.conflictFacts, "confirmed_clear");
            break;
        case TimerAssignmentPlanningConflictState::confirmedConflict:
            appendBounded(result.conflictFacts, "confirmed_conflict");
            exclude("confirmed_timer_conflict");
            break;
        case TimerAssignmentPlanningConflictState::partial:
            appendBounded(result.conflictFacts, "partial");
            exclude("conflict_evidence_partial");
            break;
        case TimerAssignmentPlanningConflictState::stale:
            appendBounded(result.conflictFacts, "stale");
            exclude("conflict_evidence_stale");
            break;
        case TimerAssignmentPlanningConflictState::unavailable:
            appendBounded(result.conflictFacts, "unavailable");
            exclude("conflict_evidence_unavailable");
            break;
    }

    if (request.role == TimerAssignmentRole::replica
        && request.intent.spec.replicaPolicy.requireBackendDiversity
        && usedBackends.find(candidate.backendId) != usedBackends.end())
    {
        exclude("backend_diversity_required");
    }

    if (request.role == TimerAssignmentRole::replica
        && request.intent.spec.replicaPolicy.requireSiteDiversity)
    {
        if (!siteEvidenceComplete || candidate.siteId.empty())
        {
            exclude("site_diversity_evidence_missing");
        }
        else if (usedSites.find(candidate.siteId) != usedSites.end())
        {
            exclude("site_diversity_required");
        }
    }

    if (request.role == TimerAssignmentRole::replacement
        && usedBackends.find(candidate.backendId) != usedBackends.end())
    {
        exclude("replacement_requires_new_backend");
    }

    result.eligible = result.exclusions.empty();
    if (result.eligible)
    {
        appendBounded(result.reasons, "eligible");
        appendBounded(
            result.reasons,
            result.preferred ? "preferred_backend" : "ordinary_backend");
    }
    return result;
}

bool sameStrings(
    const std::vector<std::string>& left,
    const std::vector<std::string>& right)
{
    return left == right;
}

bool sameChannelBinding(
    const TimerAssignmentChannelBinding& left,
    const TimerAssignmentChannelBinding& right)
{
    return left.canonicalChannelId == right.canonicalChannelId
        && left.backendChannelId == right.backendChannelId
        && left.mappingSource == right.mappingSource
        && left.mappingRevision == right.mappingRevision;
}

bool sameEvidence(
    const TimerAssignmentDecisionEvidence& left,
    const TimerAssignmentDecisionEvidence& right)
{
    return sameStrings(left.reasons, right.reasons)
        && sameStrings(left.warnings, right.warnings)
        && sameStrings(left.exclusions, right.exclusions)
        && sameStrings(left.conflictFacts, right.conflictFacts)
        && left.decisionScore == right.decisionScore;
}

bool sameCandidateEvaluation(
    const TimerAssignmentPlanningCandidateEvaluation& left,
    const TimerAssignmentPlanningCandidateEvaluation& right)
{
    return left.backendId == right.backendId
        && left.eligible == right.eligible
        && left.preferred == right.preferred
        && left.preferenceRank == right.preferenceRank
        && sameStrings(left.reasons, right.reasons)
        && sameStrings(left.exclusions, right.exclusions)
        && sameStrings(left.warnings, right.warnings)
        && sameStrings(left.conflictFacts, right.conflictFacts);
}

TimerAssignmentPlanningDecision unassignedDecision(
    const TimerAssignmentPlanningRequest& request,
    const std::string& reason)
{
    TimerAssignmentPlanningDecision decision;
    decision.outcome = TimerAssignmentPlanningOutcome::unassigned;
    decision.timerIntentId = request.intent.timerIntentId;
    decision.intentRevision = request.intent.intentRevision;
    decision.role = request.role;
    decision.decisionPolicyVersion = timerAssignmentPlanningPolicyVersion();
    appendBounded(decision.decisionEvidence.reasons, reason);
    return decision;
}

} // namespace

const char* timerAssignmentPlanningPolicyVersion()
{
    return "timer-assignment-planner/1";
}

const char* timerAssignmentPlanningBackendStateName(
    TimerAssignmentPlanningBackendState state)
{
    switch (state)
    {
        case TimerAssignmentPlanningBackendState::online: return "online";
        case TimerAssignmentPlanningBackendState::degraded: return "degraded";
        case TimerAssignmentPlanningBackendState::offline: return "offline";
        case TimerAssignmentPlanningBackendState::stale: return "stale";
        case TimerAssignmentPlanningBackendState::incompatible:
            return "incompatible";
        case TimerAssignmentPlanningBackendState::disabled: return "disabled";
    }
    return "unknown";
}

const char* timerAssignmentPlanningOutcomeName(
    TimerAssignmentPlanningOutcome outcome)
{
    switch (outcome)
    {
        case TimerAssignmentPlanningOutcome::selected: return "selected";
        case TimerAssignmentPlanningOutcome::unassigned: return "unassigned";
        case TimerAssignmentPlanningOutcome::invalid: return "invalid";
    }
    return "invalid";
}

TimerAssignmentPlanningDecision planTimerAssignment(
    const TimerAssignmentPlanningRequest& request)
{
    TimerAssignmentPlanningDecision invalid;
    invalid.timerIntentId = request.intent.timerIntentId;
    invalid.intentRevision = request.intent.intentRevision;
    invalid.role = request.role;
    invalid.decisionPolicyVersion = timerAssignmentPlanningPolicyVersion();

    if (!requestShapeValid(request))
    {
        appendBounded(invalid.decisionEvidence.reasons, "invalid_planning_input");
        return invalid;
    }

    if (request.intent.spec.assignmentPolicy.requireOperatorReview)
    {
        return unassignedDecision(request, "operator_review_required");
    }

    if (request.role == TimerAssignmentRole::primary
        && hasActivePrimary(request.currentAssignments))
    {
        return unassignedDecision(request, "active_primary_exists");
    }

    if (request.role == TimerAssignmentRole::replica
        && activeReplicaOwnerCount(request.currentAssignments)
            >= request.intent.spec.replicaPolicy.desiredAssignments)
    {
        return unassignedDecision(request, "replica_target_satisfied");
    }

    const auto usedBackends = activeBackendIds(request.currentAssignments);
    bool siteEvidenceComplete = true;
    const auto usedSites = activeSiteIds(
        request.currentAssignments,
        request.candidates,
        siteEvidenceComplete);

    std::vector<const TimerAssignmentPlanningBackendCandidate*> ordered;
    ordered.reserve(request.candidates.size());
    for (const auto& candidate : request.candidates)
    {
        ordered.push_back(&candidate);
    }

    std::sort(
        ordered.begin(),
        ordered.end(),
        [&](const auto* left, const auto* right)
        {
            bool leftPreferred = false;
            bool rightPreferred = false;
            const auto leftRank = preferenceRank(
                request.intent.spec.assignmentPolicy,
                left->backendId,
                leftPreferred);
            const auto rightRank = preferenceRank(
                request.intent.spec.assignmentPolicy,
                right->backendId,
                rightPreferred);
            if (leftRank != rightRank) return leftRank < rightRank;
            return left->backendId < right->backendId;
        });

    TimerAssignmentPlanningDecision decision;
    decision.outcome = TimerAssignmentPlanningOutcome::unassigned;
    decision.timerIntentId = request.intent.timerIntentId;
    decision.intentRevision = request.intent.intentRevision;
    decision.role = request.role;
    decision.decisionPolicyVersion = timerAssignmentPlanningPolicyVersion();

    const TimerAssignmentPlanningBackendCandidate* selected = nullptr;
    const TimerAssignmentPlanningCandidateEvaluation* selectedEvaluation = nullptr;

    for (const auto* candidate : ordered)
    {
        decision.candidates.push_back(evaluateCandidate(
            request,
            *candidate,
            usedBackends,
            usedSites,
            siteEvidenceComplete));

        const auto& evaluation = decision.candidates.back();
        if (!selected && evaluation.eligible)
        {
            selected = candidate;
            selectedEvaluation = &evaluation;
        }
    }

    for (const auto& evaluation : decision.candidates)
    {
        for (const auto& exclusion : evaluation.exclusions)
        {
            appendBounded(
                decision.decisionEvidence.exclusions,
                backendFact(evaluation.backendId, exclusion));
        }
    }

    if (!selected || !selectedEvaluation)
    {
        appendBounded(decision.decisionEvidence.reasons, "no_eligible_backend");
        return decision;
    }

    decision.outcome = TimerAssignmentPlanningOutcome::selected;
    decision.selectedBackendId = selected->backendId;
    decision.selectedBackendGeneration = selected->currentBackendGeneration;
    decision.selectedChannelBinding.canonicalChannelId =
        selected->channel.canonicalChannelId;
    decision.selectedChannelBinding.backendChannelId =
        selected->channel.backendChannelId;
    decision.selectedChannelBinding.mappingSource =
        selected->channel.mappingSource;
    decision.selectedChannelBinding.mappingRevision =
        selected->channel.mappingRevision;
    decision.selectedCapabilityRevision = selected->capability.revision;
    decision.selectedBackendHealthRevision = selected->health.revision;

    appendBounded(decision.decisionEvidence.reasons, "selected_eligible_backend");
    appendBounded(
        decision.decisionEvidence.reasons,
        selectedEvaluation->preferred
            ? "selected_preferred_backend"
            : "selected_deterministic_backend_id");

    if (!request.intent.spec.assignmentPolicy.preferredBackendIds.empty()
        && !selectedEvaluation->preferred)
    {
        appendBounded(
            decision.decisionEvidence.warnings,
            "preferred_backends_ineligible");
    }

    for (const auto& warning : selectedEvaluation->warnings)
    {
        appendBounded(decision.decisionEvidence.warnings, warning);
    }
    for (const auto& fact : selectedEvaluation->conflictFacts)
    {
        appendBounded(
            decision.decisionEvidence.conflictFacts,
            backendFact(selectedEvaluation->backendId, fact));
    }

    // decisionScore is an ordinal policy marker, not a weighted runtime score.
    // Higher values mean an earlier explicit preference. Ordinary candidates
    // share the same score and are tie-broken only by stable backendId.
    decision.decisionEvidence.decisionScore =
        -static_cast<std::int32_t>(selectedEvaluation->preferenceRank);

    return decision;
}

bool timerAssignmentPlanningDecisionEquivalent(
    const TimerAssignmentPlanningDecision& left,
    const TimerAssignmentPlanningDecision& right)
{
    if (left.outcome != right.outcome
        || left.timerIntentId != right.timerIntentId
        || left.intentRevision != right.intentRevision
        || left.role != right.role
        || left.decisionPolicyVersion != right.decisionPolicyVersion
        || left.selectedBackendId != right.selectedBackendId
        || left.selectedBackendGeneration != right.selectedBackendGeneration
        || !sameChannelBinding(
            left.selectedChannelBinding,
            right.selectedChannelBinding)
        || left.selectedCapabilityRevision != right.selectedCapabilityRevision
        || left.selectedBackendHealthRevision
            != right.selectedBackendHealthRevision
        || !sameEvidence(left.decisionEvidence, right.decisionEvidence)
        || left.candidates.size() != right.candidates.size())
    {
        return false;
    }

    for (std::size_t index = 0; index < left.candidates.size(); ++index)
    {
        if (!sameCandidateEvaluation(
                left.candidates[index],
                right.candidates[index]))
        {
            return false;
        }
    }

    if (left.outcome == TimerAssignmentPlanningOutcome::unassigned
        && (!selectedTargetEmpty(left) || !selectedTargetEmpty(right)))
    {
        return false;
    }

    return true;
}

}
