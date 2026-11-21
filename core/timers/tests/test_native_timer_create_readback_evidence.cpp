#include "NativeTimerCreateReadbackEvidence.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

static NativeTimerObservation observation(const std::string& nativeId)
{
    NativeTimerObservation value;
    value.backendId = "backend-1";
    value.backendGeneration = 7;
    value.backendNativeTimerId = nativeId;
    value.observedAt = 1000;

    auto& state = value.observedState;
    state.channelId = "S19.2E-1-1019-10301";
    state.title = "Managed Timer";
    state.directory = "VDR-Suite";
    state.day = "2026-08-17";
    state.weekdays = "-------";
    state.startTime = "0930";
    state.endTime = "1015";
    state.priority = 50;
    state.lifetime = 99;
    state.enabled = true;
    state.vps = false;

    value.observedFingerprint =
        nativeTimerObservedStateFingerprint(state);

    return value;
}

static NativeTimerCreateReadbackCandidate candidate(
    const std::string& nativeId)
{
    NativeTimerCreateReadbackCandidate value;
    value.timerAssignmentId = "assignment-1";
    value.nativeTimerBindingId = "binding-1";
    value.observation = observation(nativeId);
    return value;
}

int main()
{
    NativeTimerCreateReadbackEvidence evidence;
    evidence.backendId = "backend-1";
    evidence.backendGeneration = 7;
    evidence.observedAt = 1000;
    evidence.completeness =
        NativeTimerCreateReadbackCompleteness::complete;

    // A complete inventory may contain no managed correlation.
    assert(nativeTimerCreateReadbackEvidenceValid(evidence));

    // Same Suite correlation on two distinct native timers must remain
    // observable evidence; ambiguity is decided by the verifier.
    evidence.candidates = {
        candidate("101"),
        candidate("102")
    };
    assert(nativeTimerCreateReadbackEvidenceValid(evidence));

    auto duplicateNativeId = evidence;
    duplicateNativeId.candidates[1] = candidate("101");
    assert(!nativeTimerCreateReadbackEvidenceValid(duplicateNativeId));

    auto wrongBackend = evidence;
    wrongBackend.candidates[0].observation.backendId = "backend-2";
    assert(!nativeTimerCreateReadbackEvidenceValid(wrongBackend));

    auto wrongGeneration = evidence;
    wrongGeneration.candidates[0].observation.backendGeneration = 8;
    assert(!nativeTimerCreateReadbackEvidenceValid(wrongGeneration));

    auto wrongTime = evidence;
    wrongTime.candidates[0].observation.observedAt = 999;
    assert(!nativeTimerCreateReadbackEvidenceValid(wrongTime));

    auto incomplete = evidence;
    incomplete.completeness =
        NativeTimerCreateReadbackCompleteness::unknown;
    assert(!nativeTimerCreateReadbackEvidenceValid(incomplete));

    auto invalidCorrelation = evidence;
    invalidCorrelation.candidates[0].timerAssignmentId.clear();
    assert(!nativeTimerCreateReadbackEvidenceValid(invalidCorrelation));

    std::cout << "test_native_timer_create_readback_evidence passed\n";
    return 0;
}
