#include "NativeTimerSpecification.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

int main()
{
    NativeTimerSpecification spec;
    spec.channelId = "S19.2E-1-1019-10301";
    spec.title = "Managed Timer";
    spec.directory = "VDR-Suite";
    spec.day = "2026-08-17";
    spec.weekdays = "-------";
    spec.startTime = "930";
    spec.endTime = "1015";
    spec.priority = 50;
    spec.lifetime = 99;
    spec.enabled = true;
    spec.vps = false;

    assert(nativeTimerSpecificationValid(spec));
    const auto fingerprint = nativeTimerSpecificationFingerprint(spec);
    assert(!fingerprint.empty());

    auto padded = spec;
    padded.startTime = "0930";
    assert(nativeTimerSpecificationFingerprint(padded) == fingerprint);

    NativeTimerObservedState observed;
    observed.channelId = spec.channelId;
    observed.title = spec.title;
    observed.directory = spec.directory;
    observed.day = spec.day;
    observed.weekdays = spec.weekdays;
    observed.startTime = "0930";
    observed.endTime = spec.endTime;
    observed.priority = spec.priority;
    observed.lifetime = spec.lifetime;
    observed.enabled = spec.enabled;
    observed.vps = spec.vps;
    assert(nativeTimerObservationMatchesSpecification(spec, observed));

    observed.title = "Changed";
    assert(!nativeTimerObservationMatchesSpecification(spec, observed));
    observed.title = spec.title;

    observed.eventId = "event-99";
    observed.flags = 123;
    observed.recording = true;
    observed.pending = true;
    assert(nativeTimerObservationMatchesSpecification(spec, observed));

    observed.enabled = false;
    assert(!nativeTimerObservationMatchesSpecification(spec, observed));
    observed.enabled = spec.enabled;

    observed.vps = true;
    assert(!nativeTimerObservationMatchesSpecification(spec, observed));
    observed.vps = spec.vps;

    observed.priority = 40;
    assert(!nativeTimerObservationMatchesSpecification(spec, observed));
    observed.priority = spec.priority;

    observed.startTime = "0945";
    assert(!nativeTimerObservationMatchesSpecification(spec, observed));

    auto invalid = spec;
    invalid.weekdays = "------";
    assert(!nativeTimerSpecificationValid(invalid));

    std::cout << "test_native_timer_specification passed\n";
    return 0;
}
