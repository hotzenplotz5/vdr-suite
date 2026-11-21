#include "Database.h"
#include "MutationOperation.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBinding.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerDeleteOperationPreparationService.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerObservedState observedState()
{
    NativeTimerObservedState value;
    value.channelId = "channel:1";
    value.weekdays = "-------";
    value.startTime = "930";
    value.endTime = "1000";
    return value;
}

NativeTimerBinding makeBinding(
    const std::string& bindingId,
    const std::string& nativeId,
    NativeTimerBindingOwnership ownership = NativeTimerBindingOwnership::managed)
{
    NativeTimerBinding value;
    value.nativeTimerBindingId = bindingId;
    value.backendId = "backend:a";
    value.backendGeneration = 7;
    value.backendNativeTimerId = nativeId;
    value.timerAssignmentId = "assignment:" + bindingId;
    value.ownership = ownership;
    if (ownership == NativeTimerBindingOwnership::external)
        value.timerAssignmentId.clear();
    value.observedState = observedState();
    value.observedFingerprint =
        nativeTimerObservedStateFingerprint(value.observedState);
    value.lastObservedAt = 1900;
    return value;
}

NativeTimerDeleteOperationPreparationRequest requestFor(
    const NativeTimerBinding& binding,
    const std::string& operationId,
    const std::string& idempotencyKey)
{
    NativeTimerDeleteOperationPreparationRequest request;
    request.operationId = operationId;
    request.idempotencyKey = idempotencyKey;
    request.actorId = "actor:1";
    request.requestFingerprint = "sha256:" + operationId;
    request.nativeTimerBindingId = binding.nativeTimerBindingId;
    request.expectedBindingRevision = binding.bindingRevision;
    request.expectedBackendGeneration = binding.backendGeneration;
    request.requestedAt = 2000;
    request.deadline = 3000;
    return request;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository operationRepository(database);
    NativeTimerBindingRepository bindingRepository(database);
    assert(operationRepository.ensureSchema());
    assert(bindingRepository.ensureSchema());

    NativeTimerDeleteOperationPreparationService service(
        operationRepository, bindingRepository);

    const auto created = bindingRepository.create(
        makeBinding("binding:1", "timer:17"));
    assert(created.ok());
    const auto request = requestFor(
        created.binding, "operation:delete:1", "idem:delete:1");

    const auto prepared = service.prepare(request);
    assert(prepared.status ==
        NativeTimerDeleteOperationPreparationStatus::prepared);
    assert(prepared.operation.operationRevision == "1");
    assert(prepared.operation.state == MutationOperationState::accepted);
    assert(prepared.operation.resourceType == "NativeTimerBinding");
    assert(prepared.operation.resourceId == created.binding.nativeTimerBindingId);
    assert(prepared.operation.expectedRevision == created.binding.bindingRevision);
    assert(prepared.operation.expectedResourceFingerprint ==
        created.binding.observedFingerprint);
    assert(prepared.operation.actionFamily == "timer.delete");
    assert(prepared.operation.verificationPolicy ==
        MutationOperationVerificationPolicy::readbackRequired);
    assert(prepared.handoff.operationId == request.operationId);
    assert(prepared.handoff.operationRevision == "1");
    assert(prepared.handoff.expectedBindingRevision == created.binding.bindingRevision);
    assert(prepared.handoff.expectedNativeTimerFingerprint ==
        created.binding.observedFingerprint);
    assert(prepared.handoff.backendId == created.binding.backendId);
    assert(prepared.handoff.backendGeneration == created.binding.backendGeneration);
    assert(prepared.handoff.backendNativeTimerId == created.binding.backendNativeTimerId);
    assert(prepared.handoff.timerAssignmentId == created.binding.timerAssignmentId);

    const auto replay = service.prepare(request);
    assert(replay.status ==
        NativeTimerDeleteOperationPreparationStatus::alreadyPrepared);
    assert(replay.operation.operationRevision == "1");
    assert(replay.operation.expectedResourceFingerprint ==
        created.binding.observedFingerprint);
    assert(replay.handoff.operationRevision == "1");
    assert(replay.handoff.expectedNativeTimerFingerprint ==
        created.binding.observedFingerprint);

    const auto dispatching = operationRepository.transition(
        request.operationId,
        replay.operation.operationRevision,
        MutationOperationState::accepted,
        MutationOperationState::dispatching,
        "",
        2100);
    assert(dispatching.status == MutationOperationRepositoryStatus::ok);
    assert(service.prepare(request).status ==
        NativeTimerDeleteOperationPreparationStatus::operationStateConflict);

    auto sameScopeDifferentFingerprint = request;
    sameScopeDifferentFingerprint.operationId = "operation:delete:other";
    sameScopeDifferentFingerprint.requestFingerprint = "sha256:different";
    assert(service.prepare(sameScopeDifferentFingerprint).status ==
        NativeTimerDeleteOperationPreparationStatus::idempotencyConflict);

    auto sameOperationDifferentScope = request;
    sameOperationDifferentScope.idempotencyKey = "idem:other";
    sameOperationDifferentScope.requestFingerprint = "sha256:other";
    assert(service.prepare(sameOperationDifferentScope).status ==
        NativeTimerDeleteOperationPreparationStatus::operationConflict);

    const auto staleCreated = bindingRepository.create(
        makeBinding("binding:stale", "timer:stale"));
    assert(staleCreated.ok());
    auto staleRequest = requestFor(
        staleCreated.binding, "operation:stale", "idem:stale");
    staleRequest.expectedBindingRevision = "999";
    assert(service.prepare(staleRequest).status ==
        NativeTimerDeleteOperationPreparationStatus::bindingRevisionConflict);

    const auto generationCreated = bindingRepository.create(
        makeBinding("binding:generation", "timer:generation"));
    assert(generationCreated.ok());
    auto generationRequest = requestFor(
        generationCreated.binding, "operation:generation", "idem:generation");
    generationRequest.expectedBackendGeneration = 8;
    assert(service.prepare(generationRequest).status ==
        NativeTimerDeleteOperationPreparationStatus::generationConflict);

    const auto externalCreated = bindingRepository.create(
        makeBinding(
            "binding:external", "timer:external",
            NativeTimerBindingOwnership::external));
    assert(externalCreated.ok());
    assert(service.prepare(requestFor(
        externalCreated.binding, "operation:external", "idem:external")).status ==
        NativeTimerDeleteOperationPreparationStatus::ownershipConflict);

    const auto missingCreated = bindingRepository.create(
        makeBinding("binding:missing", "timer:missing"));
    assert(missingCreated.ok());
    NativeTimerBinding missing = missingCreated.binding;
    missing.missingSince = 1950;
    missing.lastObservedAt = 1950;
    missing.driftState = NativeTimerBindingDriftState::ambiguous;
    const auto missingUpdated = bindingRepository.update(
        missing, missingCreated.binding.bindingRevision);
    assert(missingUpdated.ok());
    assert(service.prepare(requestFor(
        missingUpdated.binding, "operation:missing", "idem:missing")).status ==
        NativeTimerDeleteOperationPreparationStatus::bindingMissing);

    const auto driftCreated = bindingRepository.create(
        makeBinding("binding:drift", "timer:drift"));
    assert(driftCreated.ok());
    NativeTimerBinding drifted = driftCreated.binding;
    drifted.driftState = NativeTimerBindingDriftState::externalFieldChange;
    const auto driftUpdated = bindingRepository.update(
        drifted, driftCreated.binding.bindingRevision);
    assert(driftUpdated.ok());
    assert(service.prepare(requestFor(
        driftUpdated.binding, "operation:drift", "idem:drift")).status ==
        NativeTimerDeleteOperationPreparationStatus::driftConflict);

    NativeTimerDeleteOperationPreparationRequest missingBinding;
    missingBinding.operationId = "operation:not-found";
    missingBinding.idempotencyKey = "idem:not-found";
    missingBinding.actorId = "actor:1";
    missingBinding.requestFingerprint = "sha256:not-found";
    missingBinding.nativeTimerBindingId = "binding:not-found";
    missingBinding.expectedBindingRevision = "1";
    missingBinding.expectedBackendGeneration = 7;
    missingBinding.requestedAt = 2000;
    assert(service.prepare(missingBinding).status ==
        NativeTimerDeleteOperationPreparationStatus::bindingNotFound);

    auto invalid = request;
    invalid.requestFingerprint.clear();
    assert(service.prepare(invalid).status ==
        NativeTimerDeleteOperationPreparationStatus::invalid);

    std::cout << "Phase 64 native Timer delete operation preparation regression passed\n";
    return 0;
}
