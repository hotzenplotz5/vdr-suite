#pragma once

#include "TimerAssignment.h"
#include "TimerIntent.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::timers
{

enum class TimerAssignmentPlanningBackendState
{
    online,
    degraded,
    offline,
    stale,
    incompatible,
    disabled
};

enum class TimerAssignmentPlanningHealthState
{
    healthy,
    degraded,
    unavailable,
    unknown
};

enum class TimerAssignmentPlanningConflictState
{
    confirmedClear,
    confirmedConflict,
    partial,
    unavailable,
    stale
};

enum class TimerAssignmentPlanningOutcome
{
    selected,
    unassigned,
    invalid
};

struct TimerAssignmentPlanningCapabilityEvidence
{
    std::uint64_t backendGeneration = 0;
    std::string revision;
    bool current = false;
    bool timerCreate = false;
    bool timerReadback = false;
};

struct TimerAssignmentPlanningHealthEvidence
{
    std::uint64_t backendGeneration = 0;
    std::string revision;
    bool current = false;
    TimerAssignmentPlanningHealthState state =
        TimerAssignmentPlanningHealthState::unknown;
    bool timerWritesAvailable = false;
};

struct TimerAssignmentPlanningChannelEvidence
{
    std::uint64_t backendGeneration = 0;
    std::string mappingRevision;
    std::string mappingSource;
    std::string canonicalChannelId;
    std::string sourceType;
    std::string sourceId;
    std::string sourceChannelId;
    std::string backendChannelId;
    bool current = false;
    bool ambiguous = false;
};

struct TimerAssignmentPlanningBackendCandidate
{
    std::string backendId;
    std::string siteId;
    std::uint64_t currentBackendGeneration = 0;
    TimerAssignmentPlanningBackendState state =
        TimerAssignmentPlanningBackendState::offline;
    bool writeAllowed = false;

    // Opaque evidence that the existing backend/provider authority boundary has
    // already authorized one exact execution path. The planner never chooses a
    // provider and never falls back between providers.
    bool executionAuthorityCurrent = false;
    std::string executionAuthorityFence;

    TimerAssignmentPlanningCapabilityEvidence capability;
    TimerAssignmentPlanningHealthEvidence health;
    TimerAssignmentPlanningChannelEvidence channel;
    TimerAssignmentPlanningConflictState conflict =
        TimerAssignmentPlanningConflictState::unavailable;
};

struct TimerAssignmentPlanningRequest
{
    TimerIntent intent;
    TimerAssignmentRole role = TimerAssignmentRole::primary;
    std::vector<TimerAssignment> currentAssignments;
    std::vector<TimerAssignmentPlanningBackendCandidate> candidates;
};

struct TimerAssignmentPlanningCandidateEvaluation
{
    std::string backendId;
    bool eligible = false;
    bool preferred = false;
    std::uint32_t preferenceRank = 0;
    std::vector<std::string> reasons;
    std::vector<std::string> exclusions;
    std::vector<std::string> warnings;
    std::vector<std::string> conflictFacts;
};

struct TimerAssignmentPlanningDecision
{
    TimerAssignmentPlanningOutcome outcome =
        TimerAssignmentPlanningOutcome::invalid;
    std::string timerIntentId;
    std::string intentRevision;
    TimerAssignmentRole role = TimerAssignmentRole::primary;
    std::string decisionPolicyVersion;

    std::string selectedBackendId;
    std::uint64_t selectedBackendGeneration = 0;
    TimerAssignmentChannelBinding selectedChannelBinding;
    std::string selectedCapabilityRevision;
    std::string selectedBackendHealthRevision;

    TimerAssignmentDecisionEvidence decisionEvidence;
    std::vector<TimerAssignmentPlanningCandidateEvaluation> candidates;
};

const char* timerAssignmentPlanningPolicyVersion();
const char* timerAssignmentPlanningBackendStateName(
    TimerAssignmentPlanningBackendState state);
const char* timerAssignmentPlanningOutcomeName(
    TimerAssignmentPlanningOutcome outcome);

TimerAssignmentPlanningDecision planTimerAssignment(
    const TimerAssignmentPlanningRequest& request);

bool timerAssignmentPlanningDecisionEquivalent(
    const TimerAssignmentPlanningDecision& left,
    const TimerAssignmentPlanningDecision& right);

}
