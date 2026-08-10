#include "NativeTimerBinding.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
NativeTimerObservedState observedState()
{
    NativeTimerObservedState state;
    state.channelId = "S19.2E-1-1019-10301";
    state.eventId = "event:ard:1001";
    state.title = "Tagesschau";
    state.directory = "News";
    state.day = "2026-08-10";
    state.weekdays = "-------";
    state.startTime = "2000";
    state.endTime = "2015";
    state.flags = 1;
    state.priority = 50;
    state.lifetime = 99;
    state.enabled = true;
    state.vps = false;
    state.recording = false;
    state.pending = true;
    return state;
}

NativeTimerBinding managedBinding()
{
    NativeTimerBinding binding;
    binding.nativeTimerBindingId = "binding:one";
    binding.bindingRevision = "7";
    binding.backendId = "backend:living-room";
    binding.backendGeneration = 12;
    binding.backendNativeTimerId = "timer:42";
    binding.timerAssignmentId = "assignment:primary";
    binding.ownership = NativeTimerBindingOwnership::managed;
    binding.observedState = observedState();
    binding.observedFingerprint =
        nativeTimerObservedStateFingerprint(binding.observedState);
    binding.lastObservedAt = 1000;
    binding.lastVerifiedOperationId = "operation:create:1";
    binding.missingSince = 0;
    binding.driftState = NativeTimerBindingDriftState::none;
    return binding;
}
}

int main()
{
    const NativeTimerObservedState state = observedState();
    assert(nativeTimerObservedStateValid(state));

    const std::string fingerprint =
        nativeTimerObservedStateFingerprint(state);
    assert(!fingerprint.empty());
    assert(fingerprint.find("native-timer-observed-state/1|") == 0);
    assert(fingerprint == nativeTimerObservedStateFingerprint(state));

    NativeTimerObservedState mapperCompatible = state;
    mapperCompatible.day.clear();
    mapperCompatible.startTime = "0";
    mapperCompatible.endTime = "930";
    assert(nativeTimerObservedStateValid(mapperCompatible));
    assert(!nativeTimerObservedStateFingerprint(mapperCompatible).empty());

    NativeTimerObservedState disabled = state;
    disabled.enabled = false;
    assert(nativeTimerObservedStateFingerprint(disabled) != fingerprint);

    NativeTimerObservedState invalidTime = state;
    invalidTime.startTime = "2460";
    assert(!nativeTimerObservedStateValid(invalidTime));
    assert(nativeTimerObservedStateFingerprint(invalidTime).empty());

    NativeTimerObservedState invalidMinute = state;
    invalidMinute.endTime = "1960";
    assert(!nativeTimerObservedStateValid(invalidMinute));

    NativeTimerObservedState invalidTimeText = state;
    invalidTimeText.startTime = "09:30";
    assert(!nativeTimerObservedStateValid(invalidTimeText));

    NativeTimerObservedState invalidWeekdays = state;
    invalidWeekdays.weekdays = "------";
    assert(!nativeTimerObservedStateValid(invalidWeekdays));

    const NativeTimerBinding managed = managedBinding();
    assert(nativeTimerBindingValid(managed));
    assert(std::string(nativeTimerBindingOwnershipName(managed.ownership)) ==
        "managed");
    assert(std::string(nativeTimerBindingDriftStateName(managed.driftState)) ==
        "none");

    NativeTimerBinding adopted = managed;
    adopted.nativeTimerBindingId = "binding:adopted";
    adopted.ownership = NativeTimerBindingOwnership::adopted;
    adopted.lastVerifiedOperationId.clear();
    assert(nativeTimerBindingValid(adopted));

    NativeTimerBinding external = managed;
    external.nativeTimerBindingId = "binding:external";
    external.timerAssignmentId.clear();
    external.ownership = NativeTimerBindingOwnership::external;
    external.lastVerifiedOperationId.clear();
    assert(nativeTimerBindingValid(external));
    assert(std::string(nativeTimerBindingOwnershipName(external.ownership)) ==
        "external");

    NativeTimerBinding externalWithAssignment = external;
    externalWithAssignment.timerAssignmentId = "assignment:illegal";
    assert(!nativeTimerBindingValid(externalWithAssignment));

    NativeTimerBinding externalWithOperation = external;
    externalWithOperation.lastVerifiedOperationId = "operation:illegal";
    assert(!nativeTimerBindingValid(externalWithOperation));

    NativeTimerBinding managedWithoutAssignment = managed;
    managedWithoutAssignment.timerAssignmentId.clear();
    assert(!nativeTimerBindingValid(managedWithoutAssignment));

    NativeTimerBinding orphaned = managed;
    orphaned.nativeTimerBindingId = "binding:orphaned";
    orphaned.ownership = NativeTimerBindingOwnership::orphanedManaged;
    orphaned.timerAssignmentId.clear();
    assert(nativeTimerBindingValid(orphaned));
    assert(std::string(nativeTimerBindingOwnershipName(orphaned.ownership)) ==
        "orphaned_managed");

    NativeTimerBinding ambiguous = managed;
    ambiguous.nativeTimerBindingId = "binding:ambiguous";
    ambiguous.ownership = NativeTimerBindingOwnership::ambiguous;
    ambiguous.timerAssignmentId.clear();
    ambiguous.driftState = NativeTimerBindingDriftState::ambiguous;
    assert(nativeTimerBindingValid(ambiguous));

    NativeTimerBinding fingerprintMismatch = managed;
    fingerprintMismatch.observedState.title = "Extern editiert";
    assert(!nativeTimerBindingValid(fingerprintMismatch));

    NativeTimerBinding missing = managed;
    missing.nativeTimerBindingId = "binding:missing";
    missing.lastObservedAt = 1200;
    missing.missingSince = 1100;
    missing.driftState = NativeTimerBindingDriftState::externalDelete;
    assert(nativeTimerBindingValid(missing));
    assert(std::string(nativeTimerBindingDriftStateName(missing.driftState)) ==
        "external_delete");

    NativeTimerBinding missingWithoutDrift = missing;
    missingWithoutDrift.driftState = NativeTimerBindingDriftState::none;
    assert(!nativeTimerBindingValid(missingWithoutDrift));

    NativeTimerBinding deleteWithoutMissing = managed;
    deleteWithoutMissing.driftState =
        NativeTimerBindingDriftState::externalDelete;
    assert(!nativeTimerBindingValid(deleteWithoutMissing));

    NativeTimerBinding expectedMissing = missing;
    expectedMissing.nativeTimerBindingId = "binding:expected-missing";
    expectedMissing.driftState =
        NativeTimerBindingDriftState::expectedTransition;
    assert(nativeTimerBindingValid(expectedMissing));

    NativeTimerBinding disabledDrift = managed;
    disabledDrift.nativeTimerBindingId = "binding:disabled";
    disabledDrift.observedState.enabled = false;
    disabledDrift.observedFingerprint =
        nativeTimerObservedStateFingerprint(disabledDrift.observedState);
    disabledDrift.driftState = NativeTimerBindingDriftState::externalDisable;
    assert(nativeTimerBindingValid(disabledDrift));

    NativeTimerBinding enabledDisableDrift = managed;
    enabledDisableDrift.driftState =
        NativeTimerBindingDriftState::externalDisable;
    assert(!nativeTimerBindingValid(enabledDisableDrift));

    NativeTimerBinding zeroGeneration = managed;
    zeroGeneration.backendGeneration = 0;
    assert(!nativeTimerBindingValid(zeroGeneration));

    NativeTimerBinding noNativeId = managed;
    noNativeId.backendNativeTimerId.clear();
    assert(!nativeTimerBindingValid(noNativeId));

    NativeTimerBinding futureMissing = missing;
    futureMissing.missingSince = futureMissing.lastObservedAt + 1;
    assert(!nativeTimerBindingValid(futureMissing));

    assert(nativeTimerBindingRevisionMatches("7", "7"));
    assert(!nativeTimerBindingRevisionMatches("7", "8"));
    assert(!nativeTimerBindingRevisionMatches("", "7"));

    std::cout << "test_native_timer_binding passed\n";
    return 0;
}
