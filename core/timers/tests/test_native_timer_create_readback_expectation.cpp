#include "NativeTimerCreateReadbackExpectation.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

static NativeTimerCreateReadbackExpectation makeExpectation()
{
    NativeTimerCreateReadbackExpectation expectation;

    expectation.operationId = "operation-1";
    expectation.operationState =
        NativeTimerReadbackOperationState::executedUnverified;
    expectation.timerAssignmentId = "assignment-1";
    expectation.nativeTimerBindingId = "binding-1";
    expectation.backendId = "backend-1";
    expectation.backendGeneration = 7;
    expectation.readbackNotBefore = 1000;

    auto& spec = expectation.expectedSpecification;
    spec.channelId = "S19.2E-1-1019-10301";
    spec.title = "Managed Timer";
    spec.directory = "VDR-Suite";
    spec.day = "2026-08-17";
    spec.weekdays = "-------";
    spec.startTime = "0930";
    spec.endTime = "1015";
    spec.priority = 50;
    spec.lifetime = 99;
    spec.enabled = true;
    spec.vps = false;

    expectation.expectedSpecificationFingerprint =
        nativeTimerSpecificationFingerprint(spec);

    return expectation;
}

int main()
{
    const auto valid = makeExpectation();
    assert(nativeTimerCreateReadbackExpectationValid(valid));

    auto unknown = valid;
    unknown.operationState =
        NativeTimerReadbackOperationState::outcomeUnknown;
    assert(nativeTimerCreateReadbackExpectationValid(unknown));

    auto badFingerprint = valid;
    badFingerprint.expectedSpecificationFingerprint = "wrong";
    assert(!nativeTimerCreateReadbackExpectationValid(badFingerprint));

    auto noAssignment = valid;
    noAssignment.timerAssignmentId.clear();
    assert(!nativeTimerCreateReadbackExpectationValid(noAssignment));

    auto noBinding = valid;
    noBinding.nativeTimerBindingId.clear();
    assert(!nativeTimerCreateReadbackExpectationValid(noBinding));

    auto noGeneration = valid;
    noGeneration.backendGeneration = 0;
    assert(!nativeTimerCreateReadbackExpectationValid(noGeneration));

    auto invalidState = valid;
    invalidState.operationState =
        static_cast<NativeTimerReadbackOperationState>(999);
    assert(!nativeTimerCreateReadbackExpectationValid(invalidState));

    auto changedSpec = valid;
    changedSpec.expectedSpecification.startTime = "0945";
    assert(!nativeTimerCreateReadbackExpectationValid(changedSpec));

    std::cout
        << "test_native_timer_create_readback_expectation passed\n";

    return 0;
}
