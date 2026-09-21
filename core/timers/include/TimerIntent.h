#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::timers
{

enum class TimerIntentType { programmeEvent, manualWindow, recurringSchedule };
enum class TimerIntentState { draft, active, paused, satisfied, cancelRequested, cancelled, expired, failed };

struct TimerIntentAutomationSource { std::string sourceType; std::string sourceId; std::string occurrenceId; };
struct TimerIntentBackendEventRef { std::string backendId; std::string channelId; std::string eventId; std::string sourceId; };
struct TimerIntentChannelRequirement { std::string canonicalChannelId; std::string sourceType; std::string sourceId; std::string sourceChannelId; };
struct TimerIntentSchedule { std::int64_t startAt = 0; std::int64_t stopAt = 0; std::string timezone; std::string recurrenceRule; };
struct TimerIntentRecordingOptions {
    std::int32_t startMarginSeconds = 0;
    std::int32_t stopMarginSeconds = 0;
    std::int32_t priority = 50;
    std::int32_t lifetimeDays = 99;
    bool vpsPreferred = false;
    std::string directoryPolicy;
    std::string namingPolicy;
    std::string retentionPolicyReference;
};
struct TimerIntentAssignmentPolicy {
    bool allowFailover = false;
    bool requireOperatorReview = false;
    std::vector<std::string> preferredBackendIds;
    std::vector<std::string> excludedBackendIds;
};
struct TimerIntentReplicaPolicy {
    std::uint32_t desiredAssignments = 1;
    bool requireBackendDiversity = false;
    bool requireSiteDiversity = false;
    bool simultaneousRecordingIntentional = false;
    std::string storagePolicyReference;
    std::string retentionPolicyReference;
    std::string rationale;
};
struct TimerIntentDuplicatePolicy { bool preventEquivalentIntent = true; bool requireOperatorReviewOnAmbiguity = true; };

struct TimerIntentSpec {
    TimerIntentType intentType = TimerIntentType::programmeEvent;
    std::string ownerActorId;
    TimerIntentAutomationSource automationSource;
    std::string programEventId;
    TimerIntentBackendEventRef backendEventRef;
    TimerIntentChannelRequirement channelRequirement;
    TimerIntentSchedule schedule;
    TimerIntentRecordingOptions recordingOptions;
    TimerIntentAssignmentPolicy assignmentPolicy;
    TimerIntentReplicaPolicy replicaPolicy;
    TimerIntentDuplicatePolicy duplicatePolicy;
};

struct TimerIntent {
    std::string timerIntentId;
    std::string intentRevision;
    TimerIntentState state = TimerIntentState::draft;
    std::string createdByActorId;
    TimerIntentSpec spec;
    std::int64_t createdAt = 0;
    std::int64_t updatedAt = 0;
    std::int64_t expiresAt = 0;
};

const char* timerIntentTypeName(TimerIntentType type);
const char* timerIntentStateName(TimerIntentState state);
bool timerIntentValidSpec(const TimerIntentSpec& spec);
bool timerIntentValid(const TimerIntent& intent);
bool timerIntentRevisionMatches(const std::string& expectedRevision, const std::string& currentRevision);
bool timerIntentCanTransition(TimerIntentState from, TimerIntentState to);
bool timerIntentAssignable(TimerIntentState state);
bool timerIntentTerminal(TimerIntentState state);
std::string timerIntentSemanticIdentity(const TimerIntentSpec& spec);

}
