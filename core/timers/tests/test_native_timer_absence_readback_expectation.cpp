#include "NativeTimerAbsenceReadbackExpectation.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
NativeTimerAbsenceReadbackExpectation expectation()
{
    NativeTimerAbsenceReadbackExpectation value;
    value.operationId = "operation:delete:1";
    value.operationState =
        NativeTimerReadbackOperationState::executedUnverified;
    value.nativeTimerBindingId = "binding:1";
    value.expectedBindingRevision = "3";
    value.backendId = "backend:a";
    value.backendGeneration = 8;
    value.backendNativeTimerId = "timer:17";
    value.readbackNotBefore = 2300;
    return value;
}
}

int main()
{
    auto value = expectation();
    assert(nativeTimerAbsenceReadbackExpectationValid(value));

    value.operationState = NativeTimerReadbackOperationState::outcomeUnknown;
    assert(nativeTimerAbsenceReadbackExpectationValid(value));

    value = expectation();
    value.operationState = static_cast<NativeTimerReadbackOperationState>(99);
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));

    value = expectation(); value.operationId.clear();
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));
    value = expectation(); value.nativeTimerBindingId.clear();
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));
    value = expectation(); value.expectedBindingRevision.clear();
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));
    value = expectation(); value.backendId.clear();
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));
    value = expectation(); value.backendGeneration = 0;
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));
    value = expectation(); value.backendNativeTimerId.clear();
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));
    value = expectation(); value.readbackNotBefore = 0;
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));

    value = expectation(); value.operationId = std::string(160, 'o');
    assert(nativeTimerAbsenceReadbackExpectationValid(value));
    value.operationId.push_back('x');
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));

    value = expectation(); value.backendNativeTimerId = std::string(160, 'n');
    assert(nativeTimerAbsenceReadbackExpectationValid(value));
    value.backendNativeTimerId.push_back('x');
    assert(!nativeTimerAbsenceReadbackExpectationValid(value));

    std::cout << "test_native_timer_absence_readback_expectation passed\n";
    return 0;
}
