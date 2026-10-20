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

    // Cursor progression remains bounded, independent of a failed candidate.
    const auto initialTail = repo.listManagedForFreshness("binding:a", 1400, 2);
    assert(initialTail.ok() && initialTail.bindings.size() == 1);
    assert(initialTail.bindings[0].nativeTimerBindingId == "binding:b");

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

    // After an Agent restart the same native Timer may have a newer
    // generation. A strict, unchanged live fingerprint rebinds it safely.
    auto nextGeneration = evidence(refreshed.binding, 1550);
    nextGeneration.backendGeneration = 8;
    const auto rebound = service.applyPresentObservation(
        nextGeneration, true);
    assert(rebound.ok() && rebound.binding.bindingRevision == "3");
    assert(rebound.binding.backendGeneration == 8);
    assert(rebound.binding.lastObservedAt == 1550);
    assert(rebound.binding.observedFingerprint ==
           managed.binding.observedFingerprint);
    assert(rebound.binding.timerAssignmentId ==
           managed.binding.timerAssignmentId);
    assert(rebound.binding.lastVerifiedOperationId ==
           managed.binding.lastVerifiedOperationId);
    // An old Agent generation must never roll back the binding.
    auto oldGeneration = evidence(rebound.binding, 1560);
    oldGeneration.backendGeneration = 7;
    assert(service.applyPresentObservation(oldGeneration, true).status ==
           NativeTimerBindingReadbackStatus::reconciliationRequired);
    assert(repo.findById("binding:a").binding.backendGeneration == 8);

    // A read cannot passively adopt an externally owned native Timer.
    assert(service.applyPresentObservation(
        evidence(external.binding, 1500), true).status ==
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
    // Even a newer Agent generation must not rewrite an accepted mutation.
    auto pendingGeneration = evidence(second.binding, 1601);
    pendingGeneration.backendGeneration = 8;
    assert(service.applyPresentObservation(pendingGeneration, true).status ==
           NativeTimerBindingReadbackStatus::repositoryConflict);
    assert(repo.findById("binding:b").binding.backendGeneration == 7);
    assert(db.execute(
        "UPDATE mutation_operations SET state='outcome_unknown';"));
    assert(service.applyPresentObservation(
        pendingGeneration, true).status ==
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

    // A drifted binding is not a refresh candidate.
    const auto tail = repo.listManagedForFreshness("binding:a", 1800, 2);
    assert(tail.ok() && tail.bindings.empty());
    std::cout << "test_native_timer_binding_passive_refresh passed\n";
}
