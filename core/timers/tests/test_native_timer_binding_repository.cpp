#include "Database.h"
#include "NativeTimerBinding.h"
#include "NativeTimerBindingRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
NativeTimerObservedState observedState()
{
    NativeTimerObservedState state;
    state.channelId = "S19.2E-1-1019-10301";
    state.eventId = "event:1";
    state.title = "News";
    state.directory = "News";
    state.day = "";
    state.weekdays = "-------";
    state.startTime = "930";
    state.endTime = "1000";
    state.flags = 1;
    state.priority = 50;
    state.lifetime = 99;
    state.enabled = true;
    state.pending = true;
    return state;
}

NativeTimerBinding externalBinding(
    const std::string& id,
    const std::string& backendId,
    const std::string& nativeId)
{
    NativeTimerBinding binding;
    binding.nativeTimerBindingId = id;
    binding.backendId = backendId;
    binding.backendGeneration = 1;
    binding.backendNativeTimerId = nativeId;
    binding.ownership = NativeTimerBindingOwnership::external;
    binding.observedState = observedState();
    binding.observedFingerprint =
        nativeTimerObservedStateFingerprint(binding.observedState);
    binding.lastObservedAt = 1000;
    binding.driftState = NativeTimerBindingDriftState::none;
    return binding;
}

NativeTimerBinding managedBinding(
    const std::string& id,
    const std::string& nativeId,
    const std::string& assignmentId)
{
    NativeTimerBinding binding =
        externalBinding(id, "backend:a", nativeId);
    binding.timerAssignmentId = assignmentId;
    binding.ownership = NativeTimerBindingOwnership::managed;
    binding.lastVerifiedOperationId = "operation:create";
    return binding;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    NativeTimerBindingRepository repository(database);
    assert(repository.ensureSchema());

    NativeTimerBinding external =
        externalBinding("binding:external", "backend:a", "timer:1");
    const auto created = repository.create(external);
    assert(created.ok());
    assert(created.binding.bindingRevision == "1");

    const auto byId = repository.findById("binding:external");
    assert(byId.ok());
    assert(byId.binding.backendNativeTimerId == "timer:1");

    const auto byNative = repository.findByBackendNativeTimer(
        "backend:a",
        "timer:1");
    assert(byNative.ok());
    assert(byNative.binding.nativeTimerBindingId == "binding:external");

    const auto repeated = repository.create(external);
    assert(
        repeated.status ==
        NativeTimerBindingRepositoryStatus::alreadyExists);
    assert(repeated.binding.bindingRevision == "1");

    NativeTimerBinding duplicateNative =
        externalBinding("binding:duplicate", "backend:a", "timer:1");
    const auto nativeConflict =
        repository.create(duplicateNative);
    assert(
        nativeConflict.status ==
        NativeTimerBindingRepositoryStatus::nativeIdentityConflict);
    assert(
        nativeConflict.binding.nativeTimerBindingId ==
        "binding:external");

    NativeTimerBinding managed =
        managedBinding("binding:managed", "timer:2", "assignment:1");
    const auto managedCreated = repository.create(managed);
    assert(managedCreated.ok());

    NativeTimerBinding secondManaged =
        managedBinding("binding:managed:2", "timer:3", "assignment:1");
    const auto assignmentConflict =
        repository.create(secondManaged);
    assert(
        assignmentConflict.status ==
        NativeTimerBindingRepositoryStatus::assignmentBindingConflict);
    assert(
        assignmentConflict.binding.nativeTimerBindingId ==
        "binding:managed");

    NativeTimerBinding ambiguous = managedBinding(
        "binding:ambiguous",
        "timer:4",
        "assignment:1");
    ambiguous.ownership = NativeTimerBindingOwnership::ambiguous;
    ambiguous.lastVerifiedOperationId.clear();
    ambiguous.driftState = NativeTimerBindingDriftState::ambiguous;
    const auto ambiguousCreated = repository.create(ambiguous);
    assert(ambiguousCreated.ok());

    const auto assignmentList =
        repository.listForAssignment("assignment:1");
    assert(assignmentList.ok());
    assert(assignmentList.bindings.size() == 2);
    assert(
        assignmentList.bindings[0].nativeTimerBindingId ==
        "binding:ambiguous");
    assert(
        assignmentList.bindings[1].nativeTimerBindingId ==
        "binding:managed");

    NativeTimerBinding next = created.binding;
    next.backendGeneration = 2;
    next.lastObservedAt = 1100;
    next.observedState.title = "News updated";
    next.observedFingerprint =
        nativeTimerObservedStateFingerprint(next.observedState);
    next.driftState =
        NativeTimerBindingDriftState::externalFieldChange;
    const auto updated = repository.update(next, "1");
    assert(updated.ok());
    assert(updated.binding.bindingRevision == "2");
    assert(updated.binding.backendGeneration == 2);
    assert(updated.binding.observedState.title == "News updated");

    const auto stale = repository.update(next, "1");
    assert(
        stale.status ==
        NativeTimerBindingRepositoryStatus::conflict);
    assert(stale.binding.bindingRevision == "2");

    NativeTimerBinding immutable = updated.binding;
    immutable.bindingRevision = "2";
    immutable.backendNativeTimerId = "timer:changed";
    const auto immutableConflict =
        repository.update(immutable, "2");
    assert(
        immutableConflict.status ==
        NativeTimerBindingRepositoryStatus::immutableConflict);

    NativeTimerBinding adoption = updated.binding;
    adoption.bindingRevision = "2";
    adoption.timerAssignmentId = "assignment:external-adoption";
    adoption.ownership = NativeTimerBindingOwnership::adopted;
    const auto adoptionConflict =
        repository.update(adoption, "2");
    assert(
        adoptionConflict.status ==
        NativeTimerBindingRepositoryStatus::immutableConflict);

    NativeTimerBinding oldGeneration = updated.binding;
    oldGeneration.bindingRevision = "2";
    oldGeneration.backendGeneration = 1;
    const auto generationConflict =
        repository.update(oldGeneration, "2");
    assert(
        generationConflict.status ==
        NativeTimerBindingRepositoryStatus::generationConflict);

    NativeTimerBinding oldObservation = updated.binding;
    oldObservation.bindingRevision = "2";
    oldObservation.lastObservedAt = 999;
    const auto observationConflict =
        repository.update(oldObservation, "2");
    assert(
        observationConflict.status ==
        NativeTimerBindingRepositoryStatus::observationConflict);

    const auto missing =
        repository.findByBackendNativeTimer("backend:a", "timer:missing");
    assert(
        missing.status ==
        NativeTimerBindingRepositoryStatus::notFound);

    NativeTimerBinding invalid = externalBinding(
        "binding:invalid",
        "backend:a",
        "timer:invalid");
    invalid.bindingRevision = "caller-owned";
    const auto invalidCreate = repository.create(invalid);
    assert(
        invalidCreate.status ==
        NativeTimerBindingRepositoryStatus::invalid);


    const std::string sharedPath =
        "/tmp/vdr-suite-native-timer-binding-repository.sqlite3";
    std::remove(sharedPath.c_str());

    Database databaseA;
    Database databaseB;
    assert(databaseA.open(sharedPath));
    assert(databaseB.open(sharedPath));

    NativeTimerBindingRepository repositoryA(databaseA);
    NativeTimerBindingRepository repositoryB(databaseB);
    assert(repositoryA.ensureSchema());
    assert(repositoryB.ensureSchema());

    NativeTimerBinding shared = externalBinding(
        "binding:shared",
        "backend:shared",
        "timer:shared");
    const auto sharedCreated = repositoryA.create(shared);
    assert(sharedCreated.ok());

    const auto sharedRead = repositoryB.findById("binding:shared");
    assert(sharedRead.ok());
    assert(sharedRead.binding.bindingRevision == "1");

    NativeTimerBinding writerA = sharedCreated.binding;
    writerA.lastObservedAt = 1200;
    writerA.backendGeneration = 2;
    writerA.observedState.title = "Writer A";
    writerA.observedFingerprint =
        nativeTimerObservedStateFingerprint(writerA.observedState);
    writerA.driftState =
        NativeTimerBindingDriftState::externalFieldChange;
    const auto writerAUpdated = repositoryA.update(writerA, "1");
    assert(writerAUpdated.ok());
    assert(writerAUpdated.binding.bindingRevision == "2");

    NativeTimerBinding staleWriterB = sharedRead.binding;
    staleWriterB.lastObservedAt = 1300;
    staleWriterB.backendGeneration = 2;
    staleWriterB.observedState.title = "Writer B";
    staleWriterB.observedFingerprint =
        nativeTimerObservedStateFingerprint(staleWriterB.observedState);
    staleWriterB.driftState =
        NativeTimerBindingDriftState::externalFieldChange;
    const auto writerBConflict = repositoryB.update(staleWriterB, "1");
    assert(
        writerBConflict.status ==
        NativeTimerBindingRepositoryStatus::conflict);
    assert(writerBConflict.binding.bindingRevision == "2");

    databaseA.close();
    databaseB.close();
    std::remove(sharedPath.c_str());

    std::cout
        << "test_native_timer_binding_repository passed\n";
    return 0;
}
