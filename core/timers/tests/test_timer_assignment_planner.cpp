#include "TimerAssignmentPlanner.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

using namespace vdrsuite::timers;

namespace
{
TimerIntent makeIntent()
{
    TimerIntent intent;
    intent.timerIntentId = "timer-intent:planner";
    intent.intentRevision = "7";
    intent.state = TimerIntentState::active;
    intent.createdByActorId = "actor:owner";
    intent.spec.intentType = TimerIntentType::manualWindow;
    intent.spec.ownerActorId = "actor:owner";
    intent.spec.channelRequirement.canonicalChannelId = "channel:news";
    intent.spec.schedule.startAt = 1000;
    intent.spec.schedule.stopAt = 2000;
    intent.spec.schedule.timezone = "Europe/Rome";
    intent.spec.replicaPolicy.desiredAssignments = 1;
    intent.createdAt = 100;
    intent.updatedAt = 100;
    return intent;
}

TimerAssignmentPlanningBackendCandidate makeCandidate(
    const std::string& backendId,
    const std::string& siteId = "site:one")
{
    TimerAssignmentPlanningBackendCandidate candidate;
    candidate.backendId = backendId;
    candidate.siteId = siteId;
    candidate.currentBackendGeneration = 4;
    candidate.state = TimerAssignmentPlanningBackendState::online;
    candidate.writeAllowed = true;
    candidate.executionAuthorityCurrent = true;
    candidate.executionAuthorityFence = "authority:vdr-native:4";
    candidate.capability.backendGeneration = 4;
    candidate.capability.revision = "cap:9";
    candidate.capability.current = true;
    candidate.capability.timerCreate = true;
    candidate.capability.timerReadback = true;
    candidate.health.backendGeneration = 4;
    candidate.health.revision = "health:11";
    candidate.health.current = true;
    candidate.health.state = TimerAssignmentPlanningHealthState::healthy;
    candidate.health.timerWritesAvailable = true;
    candidate.channel.backendGeneration = 4;
    candidate.channel.mappingRevision = "channel-map:5";
    candidate.channel.mappingSource = "agent-channel-observation";
    candidate.channel.canonicalChannelId = "channel:news";
    candidate.channel.backendChannelId = "S19.2E-1-1011-11100";
    candidate.channel.current = true;
    candidate.channel.ambiguous = false;
    candidate.conflict = TimerAssignmentPlanningConflictState::confirmedClear;
    return candidate;
}

TimerAssignment makeActiveAssignment(
    const std::string& assignmentId,
    const std::string& backendId,
    TimerAssignmentRole role)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = assignmentId;
    assignment.assignmentRevision = "3";
    assignment.timerIntentId = "timer-intent:planner";
    assignment.intentRevision = "7";
    assignment.assignmentEpoch = 2;
    assignment.backendId = backendId;
    assignment.backendGeneration = 4;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = role;
    assignment.channelBinding.canonicalChannelId = "channel:news";
    assignment.channelBinding.backendChannelId = "native:" + backendId;
    assignment.channelBinding.mappingSource = "agent-channel-observation";
    assignment.channelBinding.mappingRevision = "channel-map:4";
    assignment.capabilityRevision = "cap:8";
    assignment.backendHealthRevision = "health:10";
    assignment.decisionPolicyVersion = "timer-assignment-planner/1";
    assignment.decisionEvidence.reasons = {"selected_eligible_backend"};
    assignment.createdAt = 100;
    assignment.updatedAt = 100;
    return assignment;
}

bool has(
    const std::vector<std::string>& values,
    const std::string& expected)
{
    return std::find(values.begin(), values.end(), expected) != values.end();
}

bool containsFragment(
    const std::vector<std::string>& values,
    const std::string& fragment)
{
    return std::any_of(
        values.begin(),
        values.end(),
        [&](const std::string& value)
        {
            return value.find(fragment) != std::string::npos;
        });
}

void assertSelected(
    const TimerAssignmentPlanningDecision& decision,
    const std::string& backendId)
{
    assert(decision.outcome == TimerAssignmentPlanningOutcome::selected);
    assert(decision.timerIntentId == "timer-intent:planner");
    assert(decision.intentRevision == "7");
    assert(decision.decisionPolicyVersion == "timer-assignment-planner/1");
    assert(decision.selectedBackendId == backendId);
    assert(decision.selectedBackendGeneration == 4);
    assert(decision.selectedCapabilityRevision == "cap:9");
    assert(decision.selectedBackendHealthRevision == "health:11");
    assert(decision.selectedChannelBinding.canonicalChannelId == "channel:news");
    assert(!decision.selectedChannelBinding.backendChannelId.empty());
    assert(has(decision.decisionEvidence.reasons, "selected_eligible_backend"));
}

void assertUnassigned(
    const TimerAssignmentPlanningDecision& decision,
    const std::string& reason)
{
    assert(decision.outcome == TimerAssignmentPlanningOutcome::unassigned);
    assert(has(decision.decisionEvidence.reasons, reason));
    assert(decision.selectedBackendId.empty());
    assert(decision.selectedBackendGeneration == 0);
    assert(decision.selectedCapabilityRevision.empty());
    assert(decision.selectedBackendHealthRevision.empty());
    assert(decision.selectedChannelBinding.backendChannelId.empty());
}
}

int main()
{
    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.candidates = {
            makeCandidate("backend:b"),
            makeCandidate("backend:a")
        };

        const auto first = planTimerAssignment(request);
        const auto second = planTimerAssignment(request);
        assertSelected(first, "backend:a");
        assert(timerAssignmentPlanningDecisionEquivalent(first, second));
        assert(first.candidates.size() == 2);
        assert(first.candidates[0].backendId == "backend:a");
        assert(first.candidates[1].backendId == "backend:b");
        assert(first.decisionEvidence.decisionScore == -32);
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        auto offline = makeCandidate("backend:a");
        offline.state = TimerAssignmentPlanningBackendState::offline;
        request.candidates = {offline, makeCandidate("backend:b")};
        const auto decision = planTimerAssignment(request);
        assertSelected(decision, "backend:b");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "backend:a:backend_offline"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.intent.spec.assignmentPolicy.preferredBackendIds = {
            "backend:b", "backend:a"
        };
        request.candidates = {
            makeCandidate("backend:a"),
            makeCandidate("backend:b")
        };
        const auto decision = planTimerAssignment(request);
        assertSelected(decision, "backend:b");
        assert(decision.decisionEvidence.decisionScore == 0);
        assert(has(
            decision.decisionEvidence.reasons,
            "selected_preferred_backend"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.intent.spec.assignmentPolicy.excludedBackendIds = {
            "backend:a"
        };
        request.candidates = {
            makeCandidate("backend:a"),
            makeCandidate("backend:b")
        };
        const auto decision = planTimerAssignment(request);
        assertSelected(decision, "backend:b");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "backend:a:excluded_by_intent"));
    }

    {
        for (const auto state : {
                 TimerAssignmentPlanningBackendState::offline,
                 TimerAssignmentPlanningBackendState::stale,
                 TimerAssignmentPlanningBackendState::incompatible})
        {
            TimerAssignmentPlanningRequest request;
            request.intent = makeIntent();
            auto candidate = makeCandidate("backend:a");
            candidate.state = state;
            request.candidates = {candidate};
            const auto decision = planTimerAssignment(request);
            assertUnassigned(decision, "no_eligible_backend");
            assert(!decision.decisionEvidence.exclusions.empty());
        }
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        auto candidate = makeCandidate("backend:a");
        candidate.capability.backendGeneration = 3;
        request.candidates = {candidate};
        const auto decision = planTimerAssignment(request);
        assertUnassigned(decision, "no_eligible_backend");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "capability_generation_stale"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        auto candidate = makeCandidate("backend:a");
        candidate.capability.timerCreate = false;
        request.candidates = {candidate};
        const auto decision = planTimerAssignment(request);
        assertUnassigned(decision, "no_eligible_backend");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "timer_capability_missing"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        auto candidate = makeCandidate("backend:a");
        candidate.health.current = false;
        request.candidates = {candidate};
        const auto decision = planTimerAssignment(request);
        assertUnassigned(decision, "no_eligible_backend");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "health_missing_or_stale"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        auto candidate = makeCandidate("backend:a");
        candidate.channel.current = false;
        request.candidates = {candidate};
        const auto decision = planTimerAssignment(request);
        assertUnassigned(decision, "no_eligible_backend");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "channel_mapping_missing_or_stale"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        auto candidate = makeCandidate("backend:a");
        candidate.channel.backendGeneration = 3;
        request.candidates = {candidate};
        const auto decision = planTimerAssignment(request);
        assertUnassigned(decision, "no_eligible_backend");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "channel_mapping_generation_stale"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        auto candidate = makeCandidate("backend:a");
        candidate.executionAuthorityCurrent = false;
        candidate.executionAuthorityFence.clear();
        request.candidates = {candidate};
        const auto decision = planTimerAssignment(request);
        assertUnassigned(decision, "no_eligible_backend");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "execution_authority_unavailable"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.candidates = {};
        assertUnassigned(
            planTimerAssignment(request),
            "no_eligible_backend");
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.currentAssignments = {
            makeActiveAssignment(
                "assignment:existing-primary",
                "backend:a",
                TimerAssignmentRole::primary)
        };
        request.candidates = {makeCandidate("backend:b")};
        assertUnassigned(
            planTimerAssignment(request),
            "active_primary_exists");

        request.role = TimerAssignmentRole::replica;
        request.intent.spec.replicaPolicy.desiredAssignments = 2;
        request.intent.spec.replicaPolicy.simultaneousRecordingIntentional = true;
        request.intent.spec.replicaPolicy.rationale = "deliberate redundancy";
        const auto replica = planTimerAssignment(request);
        assertSelected(replica, "backend:b");
        assert(replica.role == TimerAssignmentRole::replica);
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.role = TimerAssignmentRole::replica;
        request.intent.spec.replicaPolicy.desiredAssignments = 2;
        request.intent.spec.replicaPolicy.simultaneousRecordingIntentional = true;
        request.intent.spec.replicaPolicy.rationale = "deliberate redundancy";
        request.intent.spec.replicaPolicy.requireBackendDiversity = true;
        request.intent.spec.replicaPolicy.requireSiteDiversity = true;
        request.currentAssignments = {
            makeActiveAssignment(
                "assignment:existing-primary",
                "backend:a",
                TimerAssignmentRole::primary)
        };
        request.candidates = {
            makeCandidate("backend:a", "site:one"),
            makeCandidate("backend:b", "site:one"),
            makeCandidate("backend:c", "site:two")
        };
        const auto decision = planTimerAssignment(request);
        assertSelected(decision, "backend:c");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "backend:a:backend_diversity_required"));
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "backend:b:site_diversity_required"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.intent.spec.assignmentPolicy.preferredBackendIds = {
            "backend:a"
        };
        auto preferred = makeCandidate("backend:a");
        preferred.state = TimerAssignmentPlanningBackendState::offline;
        request.candidates = {
            makeCandidate("backend:b"),
            preferred
        };
        const auto decision = planTimerAssignment(request);
        assertSelected(decision, "backend:b");
        assert(has(
            decision.decisionEvidence.warnings,
            "preferred_backends_ineligible"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.candidates.reserve(32);
        for (int index = 31; index >= 0; --index)
        {
            auto candidate = makeCandidate(
                "backend:" + std::to_string(index));
            candidate.state = TimerAssignmentPlanningBackendState::offline;
            request.candidates.push_back(candidate);
        }
        const auto decision = planTimerAssignment(request);
        assertUnassigned(decision, "no_eligible_backend");
        assert(decision.candidates.size() == 32);
        assert(decision.decisionEvidence.exclusions.size() == 32);
        assert(decision.decisionEvidence.exclusions.size() <= 32);
        assert(decision.candidates.front().backendId == "backend:0");
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        auto candidate = makeCandidate("backend:a");
        candidate.conflict = TimerAssignmentPlanningConflictState::stale;
        request.candidates = {candidate};
        const auto decision = planTimerAssignment(request);
        assertUnassigned(decision, "no_eligible_backend");
        assert(containsFragment(
            decision.decisionEvidence.exclusions,
            "conflict_evidence_stale"));
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.intent.spec.assignmentPolicy.requireOperatorReview = true;
        request.candidates = {makeCandidate("backend:a")};
        assertUnassigned(
            planTimerAssignment(request),
            "operator_review_required");
    }

    {
        TimerAssignmentPlanningRequest request;
        request.intent = makeIntent();
        request.role = TimerAssignmentRole::replacement;
        request.candidates = {makeCandidate("backend:b")};
        const auto invalid = planTimerAssignment(request);
        assert(invalid.outcome == TimerAssignmentPlanningOutcome::invalid);

        request.intent.spec.assignmentPolicy.allowFailover = true;
        request.currentAssignments = {
            makeActiveAssignment(
                "assignment:old",
                "backend:a",
                TimerAssignmentRole::primary)
        };
        request.candidates = {
            makeCandidate("backend:a"),
            makeCandidate("backend:b")
        };
        assertSelected(planTimerAssignment(request), "backend:b");
    }

    return 0;
}
