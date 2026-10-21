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
        "resource_id TEXT NOT NULL,state TEXT NOT NULL,"
        "operation_id TEXT,action_family TEXT,backend_id TEXT,"
        "backend_generation INTEGER,expected_revision TEXT,"
        "expected_resource_fingerprint TEXT);"));
    // Minimal durable Assignment/Intent state for same-transaction rebind.
    assert(db.execute(
        "CREATE TABLE timer_intents (timer_intent_id TEXT PRIMARY KEY,"
        "intent_revision INTEGER,state TEXT);"
        "CREATE TABLE timer_assignments (timer_assignment_id TEXT PRIMARY KEY,"
        "native_timer_binding_id TEXT,backend_id TEXT,backend_generation INTEGER,"
        "assignment_revision INTEGER,updated_at INTEGER,intent_revision TEXT,"
        "timer_intent_id TEXT,state TEXT);"
        "INSERT INTO timer_intents VALUES ('intent:a',1,'active');"
        "INSERT INTO timer_intents VALUES ('intent:b',1,'active');"
        "INSERT INTO timer_assignments VALUES "
        "('assignment:binding:a','binding:a','default',7,1,1000,'1','intent:a','bound');"
        "INSERT INTO timer_assignments VALUES "
        "('assignment:binding:b','binding:b','default',7,1,1000,'1','intent:b','bound');"));
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
    // Recover a deployment where the old fix updated only the Binding.
    assert(db.execute(
        "UPDATE timer_assignments SET backend_generation=7 "
        "WHERE timer_assignment_id='assignment:binding:a';"));
    const auto repaired = service.applyPresentObservation(
        evidence(rebound.binding, 1551), true);
    assert(repaired.ok() && repaired.binding.backendGeneration == 8);
    // Check both persisted fields using SQL constraints without crossing
    // the approved SQLite boundary for domain tests.
    assert(db.execute(
        "CREATE TABLE assert_rebound (generation INTEGER NOT NULL CHECK(generation=8),"
        "revision INTEGER NOT NULL CHECK(revision=3));"
        "INSERT INTO assert_rebound SELECT "
        "COALESCE((SELECT backend_generation FROM timer_assignments "
        "WHERE timer_assignment_id='assignment:binding:a'),-1),"
        "COALESCE((SELECT assignment_revision FROM timer_assignments "
        "WHERE timer_assignment_id='assignment:binding:a'),-1);"));
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
        "INSERT INTO mutation_operations (resource_type,resource_id,state) VALUES "
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
        "UPDATE mutation_operations SET state='succeeded';"
        "INSERT INTO mutation_operations (resource_type,resource_id,state) VALUES "
        "('TimerAssignment','assignment:binding:b','executed_unverified');"));
    assert(service.applyPresentObservation(
        evidence(second.binding, 1602), true).status ==
        NativeTimerBindingReadbackStatus::repositoryConflict);
    assert(db.execute(
        "UPDATE mutation_operations SET state='succeeded' "
        "WHERE resource_type='TimerAssignment';"));
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
    // A provider UPDATE may change the RESTfulAPI compound ID. The
    // stable binding identity can be rekeyed only with an executed durable
    // timer.update operation and optimistic binding revision.
    const auto changedId = repo.create(makeBinding(
        "binding:identity", NativeTimerBindingOwnership::managed));
    assert(changedId.ok());
    auto post = changedId.binding;
    post.backendNativeTimerId = "native:changed-identity";
    post.observedState.startTime = "2200";
    post.observedFingerprint =
        nativeTimerObservedStateFingerprint(post.observedState);
    post.lastObservedAt = 2000;
    post.lastVerifiedOperationId = "op:rename";
    assert(repo.updateVerifiedNativeIdentity(post, "1").status ==
           NativeTimerBindingRepositoryStatus::conflict);
    assert(db.execute(
        "INSERT INTO mutation_operations "
        "(resource_type,resource_id,state,operation_id,action_family,"
        "backend_id,backend_generation,expected_revision,expected_resource_fingerprint) "
        "VALUES ('NativeTimerBinding','binding:identity','executed_unverified',"
        "'op:rename','timer.update','default',7,'1','" +
        changedId.binding.observedFingerprint + "');"));
    const auto remapped = repo.updateVerifiedNativeIdentity(post, "1");
    assert(remapped.ok() && remapped.binding.bindingRevision == "2");
    assert(repo.findByBackendNativeTimer(
        "default", "native:binding:identity").status ==
        NativeTimerBindingRepositoryStatus::notFound);
    assert(repo.findByBackendNativeTimer(
        "default", "native:changed-identity").ok());
    assert(repo.updateVerifiedNativeIdentity(post, "1").status ==
           NativeTimerBindingRepositoryStatus::conflict);

    // VDR's later EPG association must be recognized as metadata-only drift.
    // Even this safe refresh cannot rewrite a binding while an UPDATE is
    // unresolved; it may proceed only after the operation becomes terminal.
    const auto originalEpgBinding = repo.findById("binding:a").binding;
    auto eventDrift = evidence(originalEpgBinding, 1900);
    eventDrift.observedState.eventId = "13546";
    eventDrift.observedFingerprint =
        nativeTimerObservedStateFingerprint(eventDrift.observedState);
    assert(eventDrift.observedFingerprint !=
           originalEpgBinding.observedFingerprint);
    assert(nativeTimerObservedStateSameExceptEventId(
        originalEpgBinding.observedState, eventDrift.observedState));

    assert(db.execute(
        "INSERT INTO mutation_operations (resource_type,resource_id,state) "
        "VALUES ('NativeTimerBinding','binding:a','outcome_unknown');"));
    assert(service.applyPresentObservation(eventDrift, true).status ==
           NativeTimerBindingReadbackStatus::repositoryConflict);
    const auto beforeTerminal = repo.findById("binding:a").binding;
    assert(beforeTerminal.observedFingerprint ==
           originalEpgBinding.observedFingerprint);
    assert(beforeTerminal.bindingRevision ==
           originalEpgBinding.bindingRevision);
    assert(db.execute(
        "UPDATE mutation_operations SET state='failed_verified' "
        "WHERE resource_id='binding:a';"));
    const auto eventRefreshed = service.applyPresentObservation(
        eventDrift, true);
    assert(eventRefreshed.ok());
    assert(eventRefreshed.binding.observedState.eventId == "13546");
    assert(eventRefreshed.binding.observedFingerprint ==
           eventDrift.observedFingerprint);
    assert(eventRefreshed.binding.lastVerifiedOperationId ==
           originalEpgBinding.lastVerifiedOperationId);

    // A title change is a real mutation, never a passive metadata refresh.
    auto titleDrift = evidence(eventRefreshed.binding, 1901);
    titleDrift.observedState.title = "Mutated";
    titleDrift.observedFingerprint =
        nativeTimerObservedStateFingerprint(titleDrift.observedState);
    assert(service.applyPresentObservation(titleDrift, true).status ==
           NativeTimerBindingReadbackStatus::reconciliationRequired);

    std::cout << "test_native_timer_binding_passive_refresh passed\n";
}
