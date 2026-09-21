#include "NativeTimerReadbackExpectation.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
NativeTimerObservedState expectedState()
{
    NativeTimerObservedState state;
    state.channelId = "S19.2E-1-1019-10301";
    state.eventId = "4242";
    state.title = "Phase 64";
    state.directory = "Series";
    state.day = "2026-08-11";
    state.weekdays = "-------";
    state.startTime = "930";
    state.endTime = "1030";
    state.flags = 1;
    state.priority = 50;
    state.lifetime = 99;
    state.enabled = true;
    state.vps = false;
    state.recording = false;
    state.pending = false;
    return state;
}

NativeTimerReadbackExpectation validExpectation()
{
    NativeTimerReadbackExpectation expectation;
    expectation.operationId = "operation-42";
    expectation.operationState =
        NativeTimerReadbackOperationState::executedUnverified;
    expectation.nativeTimerBindingId = "binding-7";
    expectation.expectedBindingRevision = "12";
    expectation.backendId = "backend-a";
    expectation.backendGeneration = 9;
    expectation.backendNativeTimerId = "17";
    expectation.readbackNotBefore = 1786428000;
    expectation.expectedState = expectedState();
    expectation.expectedFingerprint =
        nativeTimerObservedStateFingerprint(expectation.expectedState);
    return expectation;
}
}

int main()
{
    auto expectation = validExpectation();
    assert(nativeTimerReadbackExpectationValid(expectation));
    assert(std::string(nativeTimerReadbackOperationStateName(
        NativeTimerReadbackOperationState::executedUnverified))
        == "executed_unverified");

    expectation.operationState =
        NativeTimerReadbackOperationState::outcomeUnknown;
    assert(nativeTimerReadbackExpectationValid(expectation));
    assert(std::string(nativeTimerReadbackOperationStateName(
        NativeTimerReadbackOperationState::outcomeUnknown))
        == "outcome_unknown");

    expectation = validExpectation();
    expectation.operationState =
        static_cast<NativeTimerReadbackOperationState>(99);
    assert(!nativeTimerReadbackExpectationValid(expectation));

    expectation = validExpectation();
    expectation.operationId.clear();
    assert(!nativeTimerReadbackExpectationValid(expectation));

    expectation = validExpectation();
    expectation.expectedBindingRevision.clear();
    assert(!nativeTimerReadbackExpectationValid(expectation));

    expectation = validExpectation();
    expectation.backendGeneration = 0;
    assert(!nativeTimerReadbackExpectationValid(expectation));

    expectation = validExpectation();
    expectation.readbackNotBefore = 0;
    assert(!nativeTimerReadbackExpectationValid(expectation));

    expectation = validExpectation();
    expectation.expectedFingerprint += "-different";
    assert(!nativeTimerReadbackExpectationValid(expectation));

    expectation = validExpectation();
    expectation.expectedState.startTime = "09:30";
    assert(!nativeTimerReadbackExpectationValid(expectation));

    auto normalizedA = validExpectation();
    auto normalizedB = validExpectation();
    normalizedB.expectedState.startTime = "0930";
    normalizedB.expectedFingerprint =
        nativeTimerObservedStateFingerprint(normalizedB.expectedState);
    assert(normalizedA.expectedFingerprint == normalizedB.expectedFingerprint);
    assert(nativeTimerReadbackExpectationValid(normalizedB));

    expectation = validExpectation();
    expectation.backendNativeTimerId = std::string(161, 'x');
    assert(!nativeTimerReadbackExpectationValid(expectation));

    std::cout << "test_native_timer_readback_expectation passed\n";
    return 0;
}
