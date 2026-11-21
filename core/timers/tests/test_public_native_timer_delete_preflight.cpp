#include "NativeTimerDeletePublicPreflight.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

namespace
{
NativeTimerBinding observed()
{
    NativeTimerBinding b;
    b.nativeTimerBindingId = "binding-one";
    b.bindingRevision = "7";
    b.backendId = "backend-one";
    b.backendGeneration = 23;
    b.backendNativeTimerId = "native-17";
    b.timerAssignmentId = "assignment-one";
    b.ownership = NativeTimerBindingOwnership::managed;
    b.observedState.channelId = "C-1-1051-10301";
    b.observedState.day = "2026-10-20";
    b.observedState.weekdays = "-------";
    b.observedState.startTime = "2000";
    b.observedState.endTime = "2015";
    b.observedFingerprint =
        nativeTimerObservedStateFingerprint(b.observedState);
    b.lastObservedAt = 1900;
    assert(nativeTimerBindingValid(b));
    return b;
}

PublicNativeTimerDeletePreflightRequest requestFor(
    const NativeTimerBinding& b)
{
    PublicNativeTimerDeletePreflightRequest r;
    r.nativeTimerBindingId = b.nativeTimerBindingId;
    r.backendId = b.backendId;
    r.expectedBackendGeneration = b.backendGeneration;
    r.backendNativeTimerId = b.backendNativeTimerId;
    r.expectedBindingRevision = b.bindingRevision;
    r.expectedObservedFingerprint = b.observedFingerprint;
    r.observedNotBefore = 1800;
    return r;
}
}

int main()
{
    using S = PublicNativeTimerDeletePreflightStatus;
    const auto b = observed();
    const auto r = requestFor(b);
    assert(publicNativeTimerDeletePreflight(r, b) == S::eligible);

    auto adopted = b;
    adopted.ownership = NativeTimerBindingOwnership::adopted;
    assert(publicNativeTimerDeletePreflight(r, adopted) == S::eligible);

    auto external = b;
    external.ownership = NativeTimerBindingOwnership::external;
    external.timerAssignmentId.clear();
    assert(nativeTimerBindingValid(external));
    assert(publicNativeTimerDeletePreflight(r, external) ==
        S::adoptionRequired);
    auto orphan = b;
    orphan.ownership = NativeTimerBindingOwnership::orphanedManaged;
    assert(publicNativeTimerDeletePreflight(r, orphan) ==
        S::ownershipConflict);
    auto ambiguous = b;
    ambiguous.ownership = NativeTimerBindingOwnership::ambiguous;
    assert(publicNativeTimerDeletePreflight(r, ambiguous) ==
        S::ownershipConflict);

    auto invalid = r;
    invalid.observedNotBefore = 0;
    assert(publicNativeTimerDeletePreflight(invalid, b) ==
        S::invalidRequest);
    invalid = r;
    invalid.expectedObservedFingerprint.clear();
    assert(publicNativeTimerDeletePreflight(invalid, b) ==
        S::invalidRequest);

    auto changed = r;
    changed.backendId = "different";
    assert(publicNativeTimerDeletePreflight(changed, b) ==
        S::backendConflict);
    changed = r;
    changed.nativeTimerBindingId = "different";
    assert(publicNativeTimerDeletePreflight(changed, b) ==
        S::nativeIdentityConflict);
    changed = r;
    changed.backendNativeTimerId = "different";
    assert(publicNativeTimerDeletePreflight(changed, b) ==
        S::nativeIdentityConflict);
    changed = r;
    ++changed.expectedBackendGeneration;
    assert(publicNativeTimerDeletePreflight(changed, b) ==
        S::generationConflict);
    changed = r;
    changed.expectedBindingRevision = "6";
    assert(publicNativeTimerDeletePreflight(changed, b) ==
        S::revisionConflict);
    changed = r;
    changed.expectedObservedFingerprint += "changed";
    assert(publicNativeTimerDeletePreflight(changed, b) ==
        S::fingerprintConflict);
    changed = r;
    changed.observedNotBefore = b.lastObservedAt + 1;
    assert(publicNativeTimerDeletePreflight(changed, b) ==
        S::staleObservation);

    auto drift = b;
    drift.missingSince = 1850;
    drift.driftState = NativeTimerBindingDriftState::expectedTransition;
    assert(nativeTimerBindingValid(drift));
    assert(publicNativeTimerDeletePreflight(r, drift) ==
        S::nativeStateConflict);
    auto recording = b;
    recording.observedState.recording = true;
    recording.observedFingerprint =
        nativeTimerObservedStateFingerprint(recording.observedState);
    assert(nativeTimerBindingValid(recording));
    assert(publicNativeTimerDeletePreflight(requestFor(recording), recording) ==
        S::nativeStateConflict);
    auto pending = b;
    pending.observedState.pending = true;
    pending.observedFingerprint =
        nativeTimerObservedStateFingerprint(pending.observedState);
    assert(nativeTimerBindingValid(pending));
    assert(publicNativeTimerDeletePreflight(requestFor(pending), pending) ==
        S::nativeStateConflict);
    auto corrupt = b;
    corrupt.observedFingerprint = "not-canonical";
    assert(!nativeTimerBindingValid(corrupt));
    assert(publicNativeTimerDeletePreflight(r, corrupt) ==
        S::invalidEvidence);
    std::cout << "Public-v1 native Timer DELETE preflight PASS\n";
}
