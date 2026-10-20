#include "Database.h"
#include "NativeTimerBindingReadbackService.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerObservation.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace {
NativeTimerBinding makeBinding(const std::string& id,
                               NativeTimerBindingOwnership ownership)
{
    NativeTimerBinding b;
    b.nativeTimerBindingId = id;
    b.backendId = "default";
    b.backendGeneration = 7;
    b.backendNativeTimerId = "native:" + id;
    b.timerAssignmentId = ownership == NativeTimerBindingOwnership::external
        ? "" : "assignment:" + id;
    b.ownership = ownership;
    b.observedState.channelId = "channel:1";
    b.observedState.title = "News";
    b.observedState.weekdays = "-------";
    b.observedState.startTime = "2145";
    b.observedState.endTime = "2215";
    b.observedFingerprint = nativeTimerObservedStateFingerprint(b.observedState);
    b.lastObservedAt = 1000;
    b.driftState = NativeTimerBindingDriftState::none;
    return b;
}
NativeTimerObservation evidence(const NativeTimerBinding& b,
                                std::int64_t observedAt)
{
    NativeTimerObservation o;
    o.backendId = b.backendId;
    o.backendGeneration = b.backendGeneration;
    o.backendNativeTimerId = b.backendNativeTimerId;
    o.observedAt = observedAt;
    o.observedState = b.observedState;
    o.observedFingerprint = b.observedFingerprint;
    return o;
}
}

int main()
{
    Database db;
    assert(db.open(":memory:"));
    NativeTimerBindingRepository repo(db);
    assert(repo.ensureSchema());
    // The passive-refresh guard deliberately fails closed if the operation
    // authority is not present, so supply its minimal queried schema here.
    assert(db.execute(
        "CREATE TABLE mutation_operations (resource_type TEXT NOT NULL,"
        "resource_id TEXT NOT NULL,state TEXT NOT NULL);"));
    const auto managed = repo.create(makeBinding(
        "binding:a", NativeTimerBindingOwnership::managed));
    const auto second = repo.create(makeBinding(
        "binding:b", NativeTimerBindingOwnership::adopted));
    const auto external = repo.create(makeBinding(
        "binding:external", NativeTimerBindingOwnership::external));
    assert(managed.ok() && second.ok() && external.ok());
    NativeTimerBindingReadbackService service(repo);

    // 300+ seconds after CREATE the passive strict-live observation may make
    // the binding eligible again without changing the authoritative fields.
    auto candidates = repo.listManagedForFreshness("", 1400, 1);
    assert(candidates.ok() && candidates.bindings.size() == 1);
    assert(candidates.bindings.front().nativeTimerBindingId == "binding:a");
    auto refreshed = service.applyPresentObservation(
        evidence(managed.binding, 1500), true);
    assert(refreshed.ok() && refreshed.binding.bindingRevision == "2");
    assert(refreshed.binding.lastObservedAt == 1500);
    assert(refreshed.binding.observedFingerprint ==
           managed.binding.observedFingerprint);
    assert(refreshed.binding.lastVerifiedOperationId ==
           managed.binding.lastVerifiedOperationId);

    // Freshness is not adoption and cannot be used to advance generations.
    assert(service.applyPresentObservation(
        evidence(external.binding, 1500), true).status ==
        NativeTimerBindingReadbackStatus::reconciliationRequired);
    auto futureGeneration = evidence(second.binding, 1600);
    futureGeneration.backendGeneration++;
    assert(service.applyPresentObservation(futureGeneration, true).status ==
           NativeTimerBindingReadbackStatus::reconciliationRequired);

    // Changed live fields must not be hidden by refreshing the timestamp.
    auto changed = evidence(second.binding, 1600);
    changed.observedState.enabled = false;
    changed.observedFingerprint = nativeTimerObservedStateFingerprint(
        changed.observedState);
    assert(service.applyPresentObservation(changed, true).status ==
           NativeTimerBindingReadbackStatus::reconciliationRequired);

    // A reserved operation, even before dispatch, freezes the revision.
    assert(db.execute(
        "INSERT INTO mutation_operations VALUES "
        "('NativeTimerBinding','binding:b','accepted');"));
    const auto blocked = service.applyPresentObservation(
        evidence(second.binding, 1600), true);
    assert(blocked.status ==
           NativeTimerBindingReadbackStatus::repositoryConflict);
    assert(repo.findById("binding:b").binding.bindingRevision == "1");
    assert(db.execute(
        "UPDATE mutation_operations SET state='outcome_unknown';"));
    assert(service.applyPresentObservation(
        evidence(second.binding, 1601), true).status ==
           NativeTimerBindingReadbackStatus::repositoryConflict);
    assert(db.execute(
        "UPDATE mutation_operations SET state='succeeded';"));
    assert(service.applyPresentObservation(
        evidence(second.binding, 1602), true).ok());
    assert(repo.findById("binding:b").binding.bindingRevision == "2");

    // Missing/drift evidence and recording/pending prohibit passive refresh.
    auto after = repo.findById("binding:b").binding;
    after.driftState = NativeTimerBindingDriftState::externalFieldChange;
    assert(repo.update(after, "2").ok());
    assert(service.applyPresentObservation(
        evidence(after, 1700), true).status ==
           NativeTimerBindingReadbackStatus::reconciliationRequired);

    // Keyset pagination: failures of one ID cannot indefinitely starve others.
    const auto tail = repo.listManagedForFreshness("binding:a", 1400, 2);
    assert(tail.ok() && tail.bindings.size() == 1);
    assert(tail.bindings[0].nativeTimerBindingId == "binding:b");
    std::cout << "test_native_timer_binding_passive_refresh passed\n";
}
