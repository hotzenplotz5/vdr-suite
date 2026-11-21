#include "TimerIntent.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxPolicyTextLength = 256;
constexpr std::size_t kMaxBackendPolicyEntries = 32;
constexpr std::uint32_t kMaxReplicaAssignments = 4;
constexpr std::int32_t kMaxMarginSeconds = 6 * 60 * 60;
constexpr std::int32_t kMaxLifetimeDays = 3650;

bool bounded(const std::string& value, std::size_t maximum) { return value.size() <= maximum; }
bool nonEmptyBounded(const std::string& value, std::size_t maximum) { return !value.empty() && bounded(value, maximum); }

bool automationSourceValid(const TimerIntentAutomationSource& source)
{
    const bool any = !source.sourceType.empty() || !source.sourceId.empty() || !source.occurrenceId.empty();
    if (!any) return true;
    return nonEmptyBounded(source.sourceType, kMaxIdentityLength)
        && nonEmptyBounded(source.sourceId, kMaxIdentityLength)
        && bounded(source.occurrenceId, kMaxIdentityLength);
}

bool backendEventRefPresent(const TimerIntentBackendEventRef& ref)
{
    return !ref.backendId.empty() || !ref.channelId.empty() || !ref.eventId.empty() || !ref.sourceId.empty();
}

bool backendEventRefValid(const TimerIntentBackendEventRef& ref)
{
    if (!backendEventRefPresent(ref)) return true;
    return nonEmptyBounded(ref.backendId, kMaxIdentityLength)
        && nonEmptyBounded(ref.channelId, kMaxIdentityLength)
        && nonEmptyBounded(ref.eventId, kMaxIdentityLength)
        && nonEmptyBounded(ref.sourceId, kMaxIdentityLength);
}

bool channelRequirementValid(const TimerIntentChannelRequirement& channel)
{
    const bool canonical = !channel.canonicalChannelId.empty();
    const bool source = !channel.sourceType.empty() || !channel.sourceId.empty() || !channel.sourceChannelId.empty();
    if (!canonical && !source) return false;
    if (canonical && !nonEmptyBounded(channel.canonicalChannelId, kMaxIdentityLength)) return false;
    if (source && (!nonEmptyBounded(channel.sourceType, kMaxIdentityLength)
        || !nonEmptyBounded(channel.sourceId, kMaxIdentityLength)
        || !nonEmptyBounded(channel.sourceChannelId, kMaxIdentityLength))) return false;
    return true;
}

bool uniqueBoundedIds(const std::vector<std::string>& values)
{
    if (values.size() > kMaxBackendPolicyEntries) return false;
    std::set<std::string> seen;
    for (const auto& value : values) {
        if (!nonEmptyBounded(value, kMaxIdentityLength) || !seen.insert(value).second) return false;
    }
    return true;
}

bool assignmentPolicyValid(const TimerIntentAssignmentPolicy& policy)
{
    if (!uniqueBoundedIds(policy.preferredBackendIds) || !uniqueBoundedIds(policy.excludedBackendIds)) return false;
    for (const auto& backendId : policy.preferredBackendIds) {
        if (std::find(policy.excludedBackendIds.begin(), policy.excludedBackendIds.end(), backendId) != policy.excludedBackendIds.end()) return false;
    }
    return true;
}

bool recordingOptionsValid(const TimerIntentRecordingOptions& options)
{
    return options.startMarginSeconds >= 0 && options.startMarginSeconds <= kMaxMarginSeconds
        && options.stopMarginSeconds >= 0 && options.stopMarginSeconds <= kMaxMarginSeconds
        && options.priority >= 0 && options.priority <= 99
        && options.lifetimeDays >= 0 && options.lifetimeDays <= kMaxLifetimeDays
        && bounded(options.directoryPolicy, kMaxPolicyTextLength)
        && bounded(options.namingPolicy, kMaxPolicyTextLength)
        && bounded(options.retentionPolicyReference, kMaxPolicyTextLength);
}

bool scheduleValid(const TimerIntentSchedule& schedule, TimerIntentType type)
{
    if (schedule.startAt <= 0 || schedule.stopAt <= schedule.startAt || !nonEmptyBounded(schedule.timezone, 64)
        || !bounded(schedule.recurrenceRule, kMaxPolicyTextLength)) return false;
    if (type == TimerIntentType::recurringSchedule) return !schedule.recurrenceRule.empty();
    return schedule.recurrenceRule.empty();
}

void appendField(std::string& out, const std::string& value)
{
    out.append(std::to_string(value.size())); out.push_back(':'); out.append(value); out.push_back('|');
}
void appendInteger(std::string& out, std::int64_t value) { appendField(out, std::to_string(value)); }
void appendUnsigned(std::string& out, std::uint64_t value) { appendField(out, std::to_string(value)); }
void appendBool(std::string& out, bool value) { appendField(out, value ? "1" : "0"); }
void appendVector(std::string& out, const std::vector<std::string>& values)
{
    appendUnsigned(out, static_cast<std::uint64_t>(values.size()));
    for (const auto& value : values) appendField(out, value);
}
}

const char* timerIntentTypeName(TimerIntentType type)
{
    switch (type) {
    case TimerIntentType::programmeEvent: return "programme_event";
    case TimerIntentType::manualWindow: return "manual_window";
    case TimerIntentType::recurringSchedule: return "recurring_schedule";
    }
    return "unknown";
}

const char* timerIntentStateName(TimerIntentState state)
{
    switch (state) {
    case TimerIntentState::draft: return "draft";
    case TimerIntentState::active: return "active";
    case TimerIntentState::paused: return "paused";
    case TimerIntentState::satisfied: return "satisfied";
    case TimerIntentState::cancelRequested: return "cancel_requested";
    case TimerIntentState::cancelled: return "cancelled";
    case TimerIntentState::expired: return "expired";
    case TimerIntentState::failed: return "failed";
    }
    return "unknown";
}

bool timerIntentValidSpec(const TimerIntentSpec& spec)
{
    if (!nonEmptyBounded(spec.ownerActorId, kMaxIdentityLength)
        || !automationSourceValid(spec.automationSource)
        || !backendEventRefValid(spec.backendEventRef)
        || !channelRequirementValid(spec.channelRequirement)
        || !scheduleValid(spec.schedule, spec.intentType)
        || !recordingOptionsValid(spec.recordingOptions)
        || !assignmentPolicyValid(spec.assignmentPolicy)
        || spec.replicaPolicy.desiredAssignments == 0
        || spec.replicaPolicy.desiredAssignments > kMaxReplicaAssignments
        || !bounded(spec.replicaPolicy.storagePolicyReference, kMaxPolicyTextLength)
        || !bounded(spec.replicaPolicy.retentionPolicyReference, kMaxPolicyTextLength)
        || !bounded(spec.replicaPolicy.rationale, kMaxPolicyTextLength)) return false;

    if (spec.replicaPolicy.desiredAssignments > 1
        && (!spec.replicaPolicy.simultaneousRecordingIntentional
            || !nonEmptyBounded(spec.replicaPolicy.rationale, kMaxPolicyTextLength))) return false;

    const bool hasProgramEvent = !spec.programEventId.empty();
    if (hasProgramEvent && !nonEmptyBounded(spec.programEventId, kMaxIdentityLength)) return false;
    const bool hasBackendEvent = backendEventRefPresent(spec.backendEventRef);

    switch (spec.intentType) {
    case TimerIntentType::programmeEvent: return hasProgramEvent || hasBackendEvent;
    case TimerIntentType::manualWindow:
    case TimerIntentType::recurringSchedule: return !hasProgramEvent && !hasBackendEvent;
    }
    return false;
}

bool timerIntentValid(const TimerIntent& intent)
{
    return nonEmptyBounded(intent.timerIntentId, kMaxIdentityLength)
        && nonEmptyBounded(intent.intentRevision, kMaxIdentityLength)
        && nonEmptyBounded(intent.createdByActorId, kMaxIdentityLength)
        && timerIntentValidSpec(intent.spec)
        && intent.createdAt > 0
        && intent.updatedAt >= intent.createdAt
        && (intent.expiresAt == 0 || intent.expiresAt > intent.createdAt);
}

bool timerIntentRevisionMatches(const std::string& expectedRevision, const std::string& currentRevision)
{
    return !expectedRevision.empty() && !currentRevision.empty() && expectedRevision == currentRevision;
}

bool timerIntentCanTransition(TimerIntentState from, TimerIntentState to)
{
    if (from == to) return false;
    switch (from) {
    case TimerIntentState::draft:
        return to == TimerIntentState::active || to == TimerIntentState::cancelRequested || to == TimerIntentState::failed;
    case TimerIntentState::active:
        return to == TimerIntentState::paused || to == TimerIntentState::satisfied || to == TimerIntentState::cancelRequested
            || to == TimerIntentState::expired || to == TimerIntentState::failed;
    case TimerIntentState::paused:
        return to == TimerIntentState::active || to == TimerIntentState::cancelRequested || to == TimerIntentState::expired || to == TimerIntentState::failed;
    case TimerIntentState::cancelRequested:
        return to == TimerIntentState::cancelled || to == TimerIntentState::failed;
    case TimerIntentState::satisfied:
    case TimerIntentState::cancelled:
    case TimerIntentState::expired:
    case TimerIntentState::failed:
        return false;
    }
    return false;
}

bool timerIntentAssignable(TimerIntentState state) { return state == TimerIntentState::active; }
bool timerIntentTerminal(TimerIntentState state)
{
    return state == TimerIntentState::satisfied || state == TimerIntentState::cancelled
        || state == TimerIntentState::expired || state == TimerIntentState::failed;
}

std::string timerIntentSemanticIdentity(const TimerIntentSpec& spec)
{
    if (!timerIntentValidSpec(spec)) return {};
    std::string out = "timer-intent-semantic/1|";
    appendField(out, timerIntentTypeName(spec.intentType)); appendField(out, spec.ownerActorId);
    appendField(out, spec.automationSource.sourceType); appendField(out, spec.automationSource.sourceId); appendField(out, spec.automationSource.occurrenceId);
    appendField(out, spec.programEventId); appendField(out, spec.backendEventRef.backendId); appendField(out, spec.backendEventRef.channelId);
    appendField(out, spec.backendEventRef.eventId); appendField(out, spec.backendEventRef.sourceId);
    appendField(out, spec.channelRequirement.canonicalChannelId); appendField(out, spec.channelRequirement.sourceType);
    appendField(out, spec.channelRequirement.sourceId); appendField(out, spec.channelRequirement.sourceChannelId);
    appendInteger(out, spec.schedule.startAt); appendInteger(out, spec.schedule.stopAt); appendField(out, spec.schedule.timezone); appendField(out, spec.schedule.recurrenceRule);
    appendInteger(out, spec.recordingOptions.startMarginSeconds); appendInteger(out, spec.recordingOptions.stopMarginSeconds);
    appendInteger(out, spec.recordingOptions.priority); appendInteger(out, spec.recordingOptions.lifetimeDays); appendBool(out, spec.recordingOptions.vpsPreferred);
    appendField(out, spec.recordingOptions.directoryPolicy); appendField(out, spec.recordingOptions.namingPolicy); appendField(out, spec.recordingOptions.retentionPolicyReference);
    appendBool(out, spec.assignmentPolicy.allowFailover); appendBool(out, spec.assignmentPolicy.requireOperatorReview);
    appendVector(out, spec.assignmentPolicy.preferredBackendIds); appendVector(out, spec.assignmentPolicy.excludedBackendIds);
    appendUnsigned(out, spec.replicaPolicy.desiredAssignments); appendBool(out, spec.replicaPolicy.requireBackendDiversity);
    appendBool(out, spec.replicaPolicy.requireSiteDiversity); appendBool(out, spec.replicaPolicy.simultaneousRecordingIntentional);
    appendField(out, spec.replicaPolicy.storagePolicyReference); appendField(out, spec.replicaPolicy.retentionPolicyReference); appendField(out, spec.replicaPolicy.rationale);
    appendBool(out, spec.duplicatePolicy.preventEquivalentIntent); appendBool(out, spec.duplicatePolicy.requireOperatorReviewOnAmbiguity);
    return out;
}

}
