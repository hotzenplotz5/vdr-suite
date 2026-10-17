#pragma once

// Server-side typed projection from a verified EPG observation to the
// accepted ADR-0044 TimerIntentSpec. No database, HTTP or native mutation.
#include "PublicEpgTimerSelectionGuard.h"
#include "TimerIntent.h"

#include <cstdint>
#include <string>

namespace vdrsuite::public_epg_timer
{
struct ServerOwnedRecordingDefaults
{
    std::string timezone;
    std::string channelSourceType;
    std::int32_t startMarginSeconds = 0;
    std::int32_t stopMarginSeconds = 0;
    std::int32_t priority = 50;
    std::int32_t lifetimeDays = 99;
    bool vpsPreferred = false;
};

struct Draft
{
    Decision guard = Decision::invalidSelection;
    bool valid = false;
    vdrsuite::timers::TimerIntentSpec specification;
};

// The authoritative occurrence, channel source and schedule come from the
// control-plane resolver, never from a Device JSON body. An unrecognized
// native event is NOT silently promoted to a canonical programEventId.
inline Draft makeProgrammeEventDraft(
    const Selection& requested,
    const AuthoritativeObservation& observed,
    const std::string& authenticatedActorId,
    const ServerOwnedRecordingDefaults& defaults,
    bool actorHasTimersCreateGrant,
    bool backendAllowsMutation,
    std::int64_t now)
{
    Draft draft;
    draft.guard = preflight(
        requested, observed, actorHasTimersCreateGrant,
        backendAllowsMutation, now);
    if (draft.guard != Decision::ready)
        return draft;
    if (authenticatedActorId.empty() ||
        authenticatedActorId.size() > 160 ||
        defaults.channelSourceType.empty() ||
        defaults.channelSourceType.size() > 160)
        return draft;

    using namespace vdrsuite::timers;
    TimerIntentSpec spec;
    spec.intentType = TimerIntentType::programmeEvent;
    spec.ownerActorId = authenticatedActorId;
    spec.backendEventRef.backendId = observed.backendId;
    spec.backendEventRef.channelId = observed.channelId;
    spec.backendEventRef.eventId = observed.nativeEventId;
    spec.backendEventRef.sourceId = observed.sourceId;

    // Source-qualified channel reference. It is not yet a canonical
    // programme or channel identity and cannot imply backend failover.
    spec.channelRequirement.sourceType = defaults.channelSourceType;
    spec.channelRequirement.sourceId = observed.sourceId;
    spec.channelRequirement.sourceChannelId = observed.channelId;

    spec.schedule.startAt = observed.startTime;
    spec.schedule.stopAt = observed.endTime;
    spec.schedule.timezone = defaults.timezone;
    spec.recordingOptions.startMarginSeconds = defaults.startMarginSeconds;
    spec.recordingOptions.stopMarginSeconds = defaults.stopMarginSeconds;
    spec.recordingOptions.priority = defaults.priority;
    spec.recordingOptions.lifetimeDays = defaults.lifetimeDays;
    spec.recordingOptions.vpsPreferred = defaults.vpsPreferred;

    // No implicit multi-backend/replica fanout.
    spec.assignmentPolicy.preferredBackendIds = {observed.backendId};
    if (!timerIntentValidSpec(spec))
        return draft;
    draft.specification = spec;
    draft.valid = true;
    return draft;
}
} // namespace vdrsuite::public_epg_timer
