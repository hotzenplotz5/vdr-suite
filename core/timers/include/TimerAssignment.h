#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::timers
{

enum class TimerAssignmentState
{
    proposed,
    selected,
    provisioning,
    bound,
    reconciling,
    unassigned,
    superseding,
    superseded,
    cancelRequested,
    cancelled,
    failed
};

enum class TimerAssignmentRole
{
    primary,
    replica,
    replacement
};

struct TimerAssignmentChannelBinding
{
    std::string canonicalChannelId;
    std::string backendChannelId;
    std::string mappingSource;
    std::string mappingRevision;
};

struct TimerAssignmentDecisionEvidence
{
    std::vector<std::string> reasons;
    std::vector<std::string> warnings;
    std::vector<std::string> exclusions;
    std::vector<std::string> conflictFacts;
    std::int32_t decisionScore = 0;
};

struct TimerAssignment
{
    std::string timerAssignmentId;
    std::string assignmentRevision;
    std::string timerIntentId;
    std::string intentRevision;
    std::uint64_t assignmentEpoch = 0;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    TimerAssignmentState state = TimerAssignmentState::proposed;
    TimerAssignmentRole role = TimerAssignmentRole::primary;
    TimerAssignmentChannelBinding channelBinding;
    std::string capabilityRevision;
    std::string backendHealthRevision;
    std::string decisionPolicyVersion;
    TimerAssignmentDecisionEvidence decisionEvidence;
    std::string nativeTimerBindingId;
    std::int64_t createdAt = 0;
    std::int64_t updatedAt = 0;
};

const char* timerAssignmentStateName(TimerAssignmentState state);
const char* timerAssignmentRoleName(TimerAssignmentRole role);
bool timerAssignmentValid(const TimerAssignment& assignment);
bool timerAssignmentRevisionMatches(
    const std::string& expectedRevision,
    const std::string& currentRevision);
bool timerAssignmentCanTransition(
    TimerAssignmentState from,
    TimerAssignmentState to);
bool timerAssignmentActiveOwnershipState(TimerAssignmentState state);
bool timerAssignmentTerminal(TimerAssignmentState state);

}
