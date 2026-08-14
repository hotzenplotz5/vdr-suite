#include "Database.h"
#include "NativeTimerBinding.h"
#include "NativeTimerBindingReadbackService.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerObservation.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

namespace
{
NativeTimerObservedState state(const std::string& startTime = "930")
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
    value.enabled = true;
    value.pending = true;
    return value;
}

NativeTimerBinding binding(
    const std::string& id,
    const std::string& nativeId,
    std::int64_t observedAt = 1000)
{
    NativeTimerBinding value;
    value.nativeTimerBindingId = id;
    value.backendId = "backend:a";
    value.backendGeneration = 1;
    value.backendNativeTimerId = nativeId;
    value.ownership = NativeTimerBindingOwnership::external;
    value.observedState = state();
    value.observedFingerprint =
        nativeTimerObservedStateFingerprint(value.observedState);
    value.lastObservedAt = observedAt;
    value.driftState = NativeTimerBindingDriftState::none;
    return value;
}

NativeTimerObservation observation(
    const NativeTimerBinding& current,
    std::uint64_t generation,
    std::int64_t observedAt,
    const std::string& startTime = "930")
{
    NativeTimerObservation value;
    value.backendId = current.backendId;
    value.backendGeneration = generation;
    value.backendNativeTimerId = current.backendNativeTimerId;
    value.observedAt = observedAt;
    value.observedState = state(startTime);
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
    NativeTimerBindingReadbackService service(repository);

    const auto created = repository.create(binding("binding:1", "timer:1"));
    assert(created.ok());

    // Same normalized state, but newer generation/time: refresh only evidence
    // fences and preserve the durable representation instead of churning 930
    // into 0930.
    const auto padded = observation(created.binding, 2, 1100, "0930");
    assert(nativeTimerObservationValid(padded));
    assert(padded.observedFingerprint == created.binding.observedFingerprint);
    const auto refreshed = service.applyPresentObservation(padded);
    assert(refreshed.status == NativeTimerBindingReadbackStatus::refreshed);
    assert(refreshed.binding.bindingRevision == "2");
    assert(refreshed.binding.backendGeneration == 2);
    assert(refreshed.binding.lastObservedAt == 1100);
    assert(refreshed.binding.observedState.startTime == "930");
    assert(refreshed.binding.observedFingerprint == created.binding.observedFingerprint);

    const auto replay = service.applyPresentObservation(
        observation(refreshed.binding, 2, 1100, "0930"));
    assert(replay.status == NativeTimerBindingReadbackStatus::alreadyCurrent);
    assert(replay.binding.bindingRevision == "2");

    const auto staleGeneration = service.applyPresentObservation(
        observation(refreshed.binding, 1, 1200));
    assert(staleGeneration.status == NativeTimerBindingReadbackStatus::staleGeneration);
    assert(staleGeneration.binding.bindingRevision == "2");

    const auto staleTime = service.applyPresentObservation(
        observation(refreshed.binding, 2, 1099));
    assert(staleTime.status == NativeTimerBindingReadbackStatus::staleObservation);
    assert(staleTime.binding.bindingRevision == "2");

    NativeTimerObservation changed = observation(refreshed.binding, 2, 1200);
    changed.observedState.enabled = false;
    changed.observedFingerprint =
        nativeTimerObservedStateFingerprint(changed.observedState);
    const auto changedResult = service.applyPresentObservation(changed);
    assert(changedResult.status == NativeTimerBindingReadbackStatus::reconciliationRequired);
    assert(changedResult.binding.bindingRevision == "2");
    const auto afterChanged = repository.findById("binding:1");
    assert(afterChanged.ok());
    assert(afterChanged.binding.bindingRevision == "2");
    assert(afterChanged.binding.observedState.enabled);

    NativeTimerBinding missing = binding("binding:missing", "timer:missing", 1000);
    missing.lastObservedAt = 1050;
    missing.missingSince = 1050;
    missing.driftState = NativeTimerBindingDriftState::externalDelete;
    const auto missingCreated = repository.create(missing);
    assert(missingCreated.ok());
    const auto presentAfterMissing = service.applyPresentObservation(
        observation(missingCreated.binding, 1, 1100));
    assert(presentAfterMissing.status == NativeTimerBindingReadbackStatus::reconciliationRequired);
    const auto missingAfter = repository.findById("binding:missing");
    assert(missingAfter.ok());
    assert(missingAfter.binding.missingSince == 1050);
    assert(missingAfter.binding.bindingRevision == "1");

    NativeTimerBinding drifted = binding("binding:drift", "timer:drift", 1000);
    drifted.driftState = NativeTimerBindingDriftState::externalFieldChange;
    const auto driftedCreated = repository.create(drifted);
    assert(driftedCreated.ok());
    const auto driftRefresh = service.applyPresentObservation(
        observation(driftedCreated.binding, 1, 1100));
    assert(driftRefresh.status == NativeTimerBindingReadbackStatus::refreshed);
    assert(driftRefresh.binding.driftState == NativeTimerBindingDriftState::externalFieldChange);
    assert(driftRefresh.binding.bindingRevision == "2");

    NativeTimerObservation unbound;
    unbound.backendId = "backend:a";
    unbound.backendGeneration = 1;
    unbound.backendNativeTimerId = "timer:unbound";
    unbound.observedAt = 1200;
    unbound.observedState = state();
    unbound.observedFingerprint =
        nativeTimerObservedStateFingerprint(unbound.observedState);
    const auto unboundResult = service.applyPresentObservation(unbound);
    assert(unboundResult.status == NativeTimerBindingReadbackStatus::unboundObservation);
    assert(repository.findByBackendNativeTimer("backend:a", "timer:unbound").status ==
        NativeTimerBindingRepositoryStatus::notFound);

    NativeTimerObservation invalid = unbound;
    invalid.backendGeneration = 0;
    assert(service.applyPresentObservation(invalid).status ==
        NativeTimerBindingReadbackStatus::invalid);

    std::cout << "test_native_timer_binding_readback_service passed\n";
    return 0;
}
