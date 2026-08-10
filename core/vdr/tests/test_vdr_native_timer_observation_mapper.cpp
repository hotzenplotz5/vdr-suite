#include "VdrNativeTimerObservationMapper.h"

#include <cassert>
#include <iostream>

namespace
{
VdrTimer timer()
{
    VdrTimer value;
    value.id = "timer:42";
    value.channelId = "S19.2E-1-1019-10301";
    value.channelName = "Das Erste HD";
    value.eventId = "1001";
    value.title = "Tagesschau";
    value.directory = "News";
    value.subtitle = "unused subtitle";
    value.aux = "provider-private-aux";
    value.day.clear();
    value.weekdays = "-------";
    value.startTime = "930";
    value.endTime = "1000";
    value.flags = 1;
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    value.recording = false;
    value.pending = true;
    return value;
}
}

int main()
{
    const auto mapped = VdrNativeTimerObservationMapper::map(
        "backend:a", 7, 1234, timer());
    assert(mapped.ok());
    assert(mapped.observation.backendId == "backend:a");
    assert(mapped.observation.backendGeneration == 7);
    assert(mapped.observation.backendNativeTimerId == "timer:42");
    assert(mapped.observation.observedAt == 1234);
    assert(vdrsuite::timers::nativeTimerObservationValid(mapped.observation));
    assert(mapped.observation.observedState.channelId == "S19.2E-1-1019-10301");
    assert(mapped.observation.observedState.eventId == "1001");
    assert(mapped.observation.observedState.title == "Tagesschau");
    assert(mapped.observation.observedState.startTime == "930");
    assert(!mapped.observation.observedFingerprint.empty());
    assert(mapped.observation.observedFingerprint ==
        vdrsuite::timers::nativeTimerObservedStateFingerprint(
            mapped.observation.observedState));

    VdrTimer padded = timer();
    padded.startTime = "0930";
    const auto paddedMapped = VdrNativeTimerObservationMapper::map(
        "backend:a", 7, 1234, padded);
    assert(paddedMapped.ok());
    assert(paddedMapped.observation.observedState.startTime == "0930");
    assert(
        paddedMapped.observation.observedFingerprint ==
        mapped.observation.observedFingerprint);

    VdrTimer providerPrivateChanged = timer();
    providerPrivateChanged.channelName = "Renamed display name";
    providerPrivateChanged.subtitle = "different subtitle";
    providerPrivateChanged.aux = "different provider-private-aux";
    const auto providerPrivateMapped = VdrNativeTimerObservationMapper::map(
        "backend:a", 7, 1234, providerPrivateChanged);
    assert(providerPrivateMapped.ok());
    assert(
        providerPrivateMapped.observation.observedFingerprint ==
        mapped.observation.observedFingerprint);

    VdrTimer materialChanged = timer();
    materialChanged.enabled = false;
    const auto materialMapped = VdrNativeTimerObservationMapper::map(
        "backend:a", 7, 1234, materialChanged);
    assert(materialMapped.ok());
    assert(
        materialMapped.observation.observedFingerprint !=
        mapped.observation.observedFingerprint);

    VdrTimer invalidTime = timer();
    invalidTime.startTime = "2460";
    assert(
        VdrNativeTimerObservationMapper::map(
            "backend:a", 7, 1234, invalidTime).status ==
        VdrNativeTimerObservationMapStatus::invalidObservedState);

    VdrTimer noNativeId = timer();
    noNativeId.id.clear();
    assert(
        VdrNativeTimerObservationMapper::map(
            "backend:a", 7, 1234, noNativeId).status ==
        VdrNativeTimerObservationMapStatus::invalidNativeTimerIdentity);

    assert(
        VdrNativeTimerObservationMapper::map(
            "", 7, 1234, timer()).status ==
        VdrNativeTimerObservationMapStatus::invalidBackendIdentity);
    assert(
        VdrNativeTimerObservationMapper::map(
            "backend:a", 0, 1234, timer()).status ==
        VdrNativeTimerObservationMapStatus::invalidBackendGeneration);
    assert(
        VdrNativeTimerObservationMapper::map(
            "backend:a", 7, 0, timer()).status ==
        VdrNativeTimerObservationMapStatus::invalidObservedAt);

    std::cout << "test_vdr_native_timer_observation_mapper passed\n";
    return 0;
}
