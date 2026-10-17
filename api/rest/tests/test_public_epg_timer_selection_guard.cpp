#include "PublicEpgTimerSelectionGuard.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::public_epg_timer;

int main()
{
    Selection request{
        "backend-one", "S19.2E-1-1019-10302", "81234",
        "\"opaque-revision-1\"", "tv-explicit-a1"};
    AuthoritativeObservation observation{
        request.backendId, request.channelId, request.eventId,
        "vdr-native", request.ifMatch, 1792222800, 1792226400, 8U,
        true, true};
    const std::int64_t now = 1792223000;

    assert(preflight(request, observation, true, true, now) == Decision::ready);
    assert(preflight(request, observation, false, true, now) == Decision::forbidden);
    assert(preflight(request, observation, true, false, now) ==
        Decision::readOnlyBackend);
    assert(preflight(request, observation, true, true,
        observation.endTime) == Decision::eventExpired);

    Selection changed = request;
    changed.backendId = "another-backend";
    assert(preflight(changed, observation, true, true, now) ==
        Decision::scopeMismatch);
    changed = request;
    changed.channelId = "other-channel";
    assert(preflight(changed, observation, true, true, now) ==
        Decision::scopeMismatch);
    changed = request;
    changed.eventId = "81235";
    assert(preflight(changed, observation, true, true, now) ==
        Decision::scopeMismatch);
    changed = request;
    changed.ifMatch = "\"old-revision\"";
    assert(preflight(changed, observation, true, true, now) ==
        Decision::revisionConflict);
    changed = request;
    changed.eventId = "";
    assert(preflight(changed, observation, true, true, now) ==
        Decision::invalidSelection);
    changed = request;
    changed.eventId = std::string(257, 'z');
    assert(preflight(changed, observation, true, true, now) ==
        Decision::invalidSelection);
    changed = request;
    changed.ifMatch = "W/\"opaque-revision-1\"";
    assert(preflight(changed, observation, true, true, now) ==
        Decision::missingPrecondition);
    changed = request;
    changed.ifMatch = "\"bad\r\nheader\"";
    assert(preflight(changed, observation, true, true, now) ==
        Decision::missingPrecondition);
    changed = request;
    changed.idempotencyKey = "invalid key";
    assert(preflight(changed, observation, true, true, now) ==
        Decision::invalidIdempotencyKey);

    auto untrusted = observation;
    untrusted.exactNativeIdentity = false;
    assert(preflight(request, untrusted, true, true, now) ==
        Decision::noAuthoritativeObservation);
    untrusted = observation;
    untrusted.observationCurrent = false;
    assert(preflight(request, untrusted, true, true, now) ==
        Decision::noAuthoritativeObservation);
    untrusted = observation;
    untrusted.backendGeneration = 0U;
    assert(preflight(request, untrusted, true, true, now) ==
        Decision::noAuthoritativeObservation);
    untrusted = observation;
    untrusted.sourceId = "";
    assert(preflight(request, untrusted, true, true, now) ==
        Decision::noAuthoritativeObservation);
    untrusted = observation;
    untrusted.resourceEtag = "\"another-observation\"";
    assert(preflight(request, untrusted, true, true, now) ==
        Decision::revisionConflict);
    untrusted = observation;
    untrusted.endTime = untrusted.startTime;
    assert(preflight(request, untrusted, true, true, now) ==
        Decision::noAuthoritativeObservation);

    // The result is only a gate: this test has zero repositories,
    // TimerAssignments, Operation writes and VDR calls.
    std::cout << "test_public_epg_timer_selection_guard passed\n";
    return 0;
}
