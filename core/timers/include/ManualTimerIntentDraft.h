#pragma once

// F5.5.5: Pure projection of a server-verified manual native schedule into
// the existing ADR-0044 TimerIntent and native Assignment specification.
// This is not an HTTP admission, authorization decision, writer or clock
// resolver. No public manual CREATE may execute until the existing atomic
// Operation path is wired and the backend supplies current authority.
#include "NativeTimerSpecification.h"
#include "TimerIntent.h"

#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

struct ManualTimerSourceEvidence
{
    std::string backendId;
    std::string sourceType;
    std::string sourceId;
    std::string backendChannelId;
    std::string timezone;
    std::int64_t startAt = 0;
    std::int64_t stopAt = 0;
};

struct ManualTimerIntentDraft
{
    bool valid = false;
    TimerIntentSpec intent;
    NativeTimerSpecification desiredNativeTimer;
};

// The caller must obtain the source identity, exact channel binding,
// timezone and Unix timestamps from the authoritative backend. The native
// HHMM/day fields are VDR-local; they must not be converted with the daemon
// host's timezone. The complete native specification travels separately
// into the existing TimerAssignment, not into an invented intent field.
inline ManualTimerIntentDraft makeManualWindowTimerIntentDraft(
    const NativeTimerSpecification& requested,
    const ManualTimerSourceEvidence& source,
    const std::string& authenticatedActorId)
{
    ManualTimerIntentDraft draft;
    if (!nativeTimerSpecificationValid(requested)
        || requested.title.empty()
        || requested.day.empty()
        || requested.weekdays != "-------"
        || authenticatedActorId.empty()
        || source.backendId.empty()
        || source.sourceType.empty()
        || source.sourceId.empty()
        || source.backendChannelId != requested.channelId)
    {
        return draft;
    }

    TimerIntentSpec intent;
    intent.intentType = TimerIntentType::manualWindow;
    intent.ownerActorId = authenticatedActorId;
    intent.channelRequirement.sourceType = source.sourceType;
    intent.channelRequirement.sourceId = source.sourceId;
    intent.channelRequirement.sourceChannelId = source.backendChannelId;
    intent.schedule.startAt = source.startAt;
    intent.schedule.stopAt = source.stopAt;
    intent.schedule.timezone = source.timezone;
    intent.recordingOptions.priority = requested.priority;
    intent.recordingOptions.lifetimeDays = requested.lifetime;
    intent.recordingOptions.vpsPreferred = requested.vps;
    intent.assignmentPolicy.preferredBackendIds = {source.backendId};

    if (!timerIntentValidSpec(intent))
        return draft;

    draft.valid = true;
    draft.intent = intent;
    draft.desiredNativeTimer = requested;
    return draft;
}

// F5.6: VDR owns the recurring wall-clock schedule. Map the seven native
// weekday positions (Monday through Sunday) into an unambiguous weekly
// recurrence description on the existing TimerIntent; never invent a
// daemon-side recurrence scheduler or infer the VDR timezone.
inline ManualTimerIntentDraft makeRecurringTimerIntentDraft(
    const NativeTimerSpecification& requested,
    const ManualTimerSourceEvidence& source,
    const std::string& authenticatedActorId)
{
    ManualTimerIntentDraft draft;
    if (!nativeTimerSpecificationValid(requested)
        || requested.title.empty()
        || requested.day.empty()
        || requested.weekdays.size() != 7U
        || requested.weekdays == "-------"
        || authenticatedActorId.empty()
        || source.backendId.empty()
        || source.sourceType.empty()
        || source.sourceId.empty()
        || source.backendChannelId != requested.channelId
        || source.timezone.empty()
        || source.startAt <= 0
        || source.stopAt <= source.startAt
        || source.stopAt - source.startAt >= 24 * 3600)
        return draft;

    constexpr char kVdrWeekdays[] = "MTWTFSS";
    constexpr const char* kDays[] = {
        "MO", "TU", "WE", "TH", "FR", "SA", "SU"
    };
    std::string rule = "FREQ=WEEKLY;BYDAY=";
    bool any = false;
    for (std::size_t day = 0U; day < 7U; ++day)
    {
        const char value = requested.weekdays[day];
        if (value == '-') continue;
        if (value != kVdrWeekdays[day])
            return draft;
        if (any) rule += ",";
        rule += kDays[day];
        any = true;
    }
    if (!any) return draft;

    TimerIntentSpec intent;
    intent.intentType = TimerIntentType::recurringSchedule;
    intent.ownerActorId = authenticatedActorId;
    intent.channelRequirement.sourceType = source.sourceType;
    intent.channelRequirement.sourceId = source.sourceId;
    intent.channelRequirement.sourceChannelId = source.backendChannelId;
    // These UTC boundaries describe the server-attested first occurrence;
    // the recurring native timer continues to follow VDR-local weekdays.
    intent.schedule.startAt = source.startAt;
    intent.schedule.stopAt = source.stopAt;
    intent.schedule.timezone = source.timezone;
    intent.schedule.recurrenceRule = rule;
    intent.recordingOptions.priority = requested.priority;
    intent.recordingOptions.lifetimeDays = requested.lifetime;
    intent.recordingOptions.vpsPreferred = requested.vps;
    intent.assignmentPolicy.preferredBackendIds = {source.backendId};
    if (!timerIntentValidSpec(intent))
        return draft;

    draft.valid = true;
    draft.intent = intent;
    draft.desiredNativeTimer = requested;
    return draft;
}

} // namespace vdrsuite::timers
