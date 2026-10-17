#include "EpgTimerIntentDraft.h"
#include <cassert>
#include <iostream>

using namespace vdrsuite::public_epg_timer;
using namespace vdrsuite::timers;

int main()
{
    const Selection request{
        "backend-one", "S19.2E-1-1019-10302", "81234",
        "\"opaque-revision-1\"", "tv-explicit-a1"};
    const AuthoritativeObservation observation{
        request.backendId, request.channelId, request.eventId,
        "vdr-native", request.ifMatch,
        1792222800, 1792226400, 8U, true, true};
    ServerOwnedRecordingDefaults defaults;
    defaults.timezone = "Europe/Berlin";
    defaults.channelSourceType = "vdr";
    defaults.startMarginSeconds = 120;
    defaults.stopMarginSeconds = 300;
    const std::int64_t now = 1792223000;
    const auto result = makeProgrammeEventDraft(
        request, observation, "device-actor-1",
        defaults, true, true, now);
    assert(result.guard == Decision::ready);
    assert(result.valid);
    const auto& spec = result.specification;
    assert(timerIntentValidSpec(spec));
    assert(spec.intentType == TimerIntentType::programmeEvent);
    assert(spec.programEventId.empty());
    assert(spec.ownerActorId == "device-actor-1");
    assert(spec.backendEventRef.backendId == observation.backendId);
    assert(spec.backendEventRef.channelId == observation.channelId);
    assert(spec.backendEventRef.eventId == observation.nativeEventId);
    assert(spec.backendEventRef.sourceId == observation.sourceId);
    assert(spec.channelRequirement.sourceType == "vdr");
    assert(spec.channelRequirement.sourceId == observation.sourceId);
    assert(spec.channelRequirement.sourceChannelId == observation.channelId);
    assert(spec.schedule.startAt == observation.startTime);
    assert(spec.schedule.stopAt == observation.endTime);
    assert(spec.schedule.timezone == defaults.timezone);
    assert(spec.recordingOptions.startMarginSeconds == 120);
    assert(spec.recordingOptions.stopMarginSeconds == 300);
    assert(spec.assignmentPolicy.preferredBackendIds.size() == 1);
    assert(spec.assignmentPolicy.preferredBackendIds.at(0) ==
        observation.backendId);
    assert(!spec.assignmentPolicy.allowFailover);
    assert(spec.replicaPolicy.desiredAssignments == 1);
    assert(spec.duplicatePolicy.preventEquivalentIntent);
    assert(!timerIntentSemanticIdentity(spec).empty());

    auto invalidObservation = observation;
    invalidObservation.exactNativeIdentity = false;
    const auto missing = makeProgrammeEventDraft(
        request, invalidObservation, "device-actor-1",
        defaults, true, true, now);
    assert(!missing.valid);
    assert(missing.guard == Decision::noAuthoritativeObservation);
    assert(missing.specification.ownerActorId.empty());

    const auto forbidden = makeProgrammeEventDraft(
        request, observation, "device-actor-1", defaults,
        false, true, now);
    assert(!forbidden.valid);
    assert(forbidden.guard == Decision::forbidden);

    auto stale = request;
    stale.ifMatch = "\"old-revision\"";
    const auto conflict = makeProgrammeEventDraft(
        stale, observation, "device-actor-1", defaults,
        true, true, now);
    assert(!conflict.valid);
    assert(conflict.guard == Decision::revisionConflict);

    defaults.timezone = "";
    assert(!makeProgrammeEventDraft(
        request, observation, "device-actor-1", defaults,
        true, true, now).valid);
    defaults.timezone = "Europe/Berlin";
    defaults.priority = 200; // Invalid server policy cannot be excused
    assert(!makeProgrammeEventDraft(
        request, observation, "device-actor-1", defaults,
        true, true, now).valid);

    std::cout << "test_public_epg_timer_intent_draft passed\n";
    return 0;
}
