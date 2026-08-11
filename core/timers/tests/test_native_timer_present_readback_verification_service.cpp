#include "Database.h"
#include "NativeTimerBinding.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerObservation.h"
#include "NativeTimerPresentReadbackVerificationService.h"
#include "NativeTimerReadbackExpectation.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

namespace
{
NativeTimerObservedState state(const std::string& startTime = "930", bool enabled = true)
{
    NativeTimerObservedState value;
    value.channelId = "channel:1";
    value.eventId = "event:1";
    value.title = "News";
    value.directory = "News";
    value.day.clear();
    value.weekdays = "-------";
    value.startTime = startTime;
    value.endTime = "1000";
    value.flags = 1;
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = enabled;
    value.pending = true;
    return value;
}

NativeTimerBinding binding(
    const std::string& id = "binding:1",
    NativeTimerBindingOwnership ownership = NativeTimerBindingOwnership::managed)
{
    NativeTimerBinding value;
    value.nativeTimerBindingId = id;
    value.backendId = "backend:a";
    value.backendGeneration = 7;
    value.backendNativeTimerId = "timer:17";
    value.timerAssignmentId = "assignment:1";
    value.ownership = ownership;
    value.observedState = state("900", false);
    value.observedFingerprint =
        nativeTimerObservedStateFingerprint(value.observedState);
    value.lastObservedAt = 1900;
    value.missingSince = 1850;
    value.driftState = NativeTimerBindingDriftState::expectedTransition;
    return value;
}

NativeTimerReadbackExpectation expectation(
    const NativeTimerBinding& current,
    const NativeTimerObservedState& expected = state())
{
    NativeTimerReadbackExpectation value;
    value.operationId = "operation:1";
    value.operationState =
        NativeTimerReadbackOperationState::executedUnverified;
    value.nativeTimerBindingId = current.nativeTimerBindingId;
    value.expectedBindingRevision = current.bindingRevision;
    value.backendId = current.backendId;
    value.backendGeneration = 8;
    value.backendNativeTimerId = current.backendNativeTimerId;
    value.readbackNotBefore = 2000;
    value.expectedState = expected;
    value.expectedFingerprint =
        nativeTimerObservedStateFingerprint(value.expectedState);
    return value;
}

NativeTimerObservation observation(
    const NativeTimerReadbackExpectation& expected,
    std::int64_t observedAt = 2100,
    const std::string& startTime = "0930")
{
    NativeTimerObservation value;
    value.backendId = expected.backendId;
    value.backendGeneration = expected.backendGeneration;
    value.backendNativeTimerId = expected.backendNativeTimerId;
    value.observedAt = observedAt;
    value.observedState = expected.expectedState;
    value.observedState.startTime = startTime;
    value.observedFingerprint =
        nativeTimerObservedStateFingerprint(value.observedState);
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    NativeTimerBindingRepository repository(database);
    assert(repository.ensureSchema());
    NativeTimerPresentReadbackVerificationService service(repository);

    const auto created = repository.create(binding());
    assert(created.ok());
    assert(created.binding.bindingRevision == "1");

    const auto expected = expectation(created.binding);
    const auto observed = observation(expected);
    assert(observed.observedFingerprint == expected.expectedFingerprint);

    const auto verified = service.verify(expected, observed);
    assert(verified.status ==
        NativeTimerPresentReadbackVerificationStatus::verified);
    assert(verified.binding.bindingRevision == "2");
    assert(verified.binding.backendGeneration == 8);
    assert(verified.binding.lastObservedAt == 2100);
    assert(verified.binding.observedState.startTime == "0930");
    assert(verified.binding.lastVerifiedOperationId == "operation:1");
    assert(verified.binding.missingSince == 0);
    assert(verified.binding.driftState == NativeTimerBindingDriftState::none);

    const auto replay = service.verify(expected, observed);
    assert(replay.status ==
        NativeTimerPresentReadbackVerificationStatus::alreadyVerified);
    assert(replay.binding.bindingRevision == "2");

    NativeTimerObservation changedAfterVerified = observed;
    changedAfterVerified.observedAt = 2200;
    changedAfterVerified.observedState.enabled = false;
    changedAfterVerified.observedFingerprint =
        nativeTimerObservedStateFingerprint(changedAfterVerified.observedState);
    assert(service.verify(expected, changedAfterVerified).status ==
        NativeTimerPresentReadbackVerificationStatus::reconciliationRequired);

    NativeTimerReadbackExpectation staleRevision = expected;
    staleRevision.operationId = "operation:2";
    const auto staleRevisionResult = service.verify(
        staleRevision, observation(staleRevision, 2200));
    assert(staleRevisionResult.status ==
        NativeTimerPresentReadbackVerificationStatus::bindingRevisionConflict);

    NativeTimerReadbackExpectation currentExpectation = expectation(verified.binding);
    currentExpectation.operationId = "operation:2";
    currentExpectation.expectedBindingRevision = "2";
    NativeTimerObservation staleTime = observation(currentExpectation, 1999);
    const auto staleTimeResult = service.verify(currentExpectation, staleTime);
    assert(staleTimeResult.status ==
        NativeTimerPresentReadbackVerificationStatus::staleObservation);

    NativeTimerObservation wrongGeneration =
        observation(currentExpectation, 2200);
    wrongGeneration.backendGeneration = 9;
    const auto wrongGenerationResult =
        service.verify(currentExpectation, wrongGeneration);
    assert(wrongGenerationResult.status ==
        NativeTimerPresentReadbackVerificationStatus::generationConflict);

    NativeTimerObservation changed = observation(currentExpectation, 2200);
    changed.observedState.enabled = false;
    changed.observedFingerprint =
        nativeTimerObservedStateFingerprint(changed.observedState);
    const auto changedResult = service.verify(currentExpectation, changed);
    assert(changedResult.status ==
        NativeTimerPresentReadbackVerificationStatus::reconciliationRequired);
    assert(repository.findById("binding:1").binding.bindingRevision == "2");

    NativeTimerReadbackExpectation wrongIdentity = currentExpectation;
    wrongIdentity.backendNativeTimerId = "timer:other";
    NativeTimerObservation wrongIdentityObservation =
        observation(currentExpectation, 2200);
    const auto wrongIdentityResult =
        service.verify(wrongIdentity, wrongIdentityObservation);
    assert(wrongIdentityResult.status ==
        NativeTimerPresentReadbackVerificationStatus::identityConflict);

    NativeTimerBinding external = binding(
        "binding:external", NativeTimerBindingOwnership::external);
    external.timerAssignmentId.clear();
    external.missingSince = 0;
    external.driftState = NativeTimerBindingDriftState::none;
    const auto externalCreated = repository.create(external);
    assert(externalCreated.ok());
    auto externalExpectation = expectation(externalCreated.binding);
    externalExpectation.nativeTimerBindingId = externalCreated.binding.nativeTimerBindingId;
    externalExpectation.expectedBindingRevision = externalCreated.binding.bindingRevision;
    externalExpectation.backendNativeTimerId = externalCreated.binding.backendNativeTimerId;
    const auto externalObservation = observation(externalExpectation);
    assert(service.verify(externalExpectation, externalObservation).status ==
        NativeTimerPresentReadbackVerificationStatus::ownershipConflict);

    NativeTimerReadbackExpectation invalid = currentExpectation;
    invalid.readbackNotBefore = 0;
    assert(service.verify(invalid, observation(currentExpectation)).status ==
        NativeTimerPresentReadbackVerificationStatus::invalid);

    NativeTimerReadbackExpectation missingExpectation = currentExpectation;
    missingExpectation.nativeTimerBindingId = "binding:missing";
    assert(service.verify(
        missingExpectation, observation(currentExpectation)).status ==
        NativeTimerPresentReadbackVerificationStatus::bindingNotFound);

    std::cout << "test_native_timer_present_readback_verification_service passed\n";
    return 0;
}
