#include "Database.h"
#include "MutationOperation.h"
#include "MutationOperationRepository.h"
#include "NativeTimerAbsenceReadbackExpectation.h"
#include "NativeTimerBinding.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerDeleteOperationCompletionService.h"

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

NativeTimerBinding binding(
    const std::string& bindingId,
    const std::string& nativeId)
{
    NativeTimerBinding value;
    value.nativeTimerBindingId = bindingId;
    value.backendId = "backend:a";
    value.backendGeneration = 7;
    value.backendNativeTimerId = nativeId;
    value.timerAssignmentId = "assignment:" + bindingId;
    value.ownership = NativeTimerBindingOwnership::managed;
    value.observedState = observedState();
    value.observedFingerprint = nativeTimerObservedStateFingerprint(value.observedState);
    value.lastObservedAt = 1900;
    return value;
}

NativeTimerAbsenceReadbackExpectation expectation(
    const NativeTimerBinding& created,
    const std::string& operationId,
    NativeTimerReadbackOperationState state =
        NativeTimerReadbackOperationState::executedUnverified)
{
    NativeTimerAbsenceReadbackExpectation value;
    value.operationId = operationId;
    value.operationState = state;
    value.nativeTimerBindingId = created.nativeTimerBindingId;
    value.expectedBindingRevision = created.bindingRevision;
    value.backendId = created.backendId;
    value.backendGeneration = 8;
    value.backendNativeTimerId = created.backendNativeTimerId;
    value.readbackNotBefore = 2000;
    return value;
}

MutationOperation operation(
    const NativeTimerAbsenceReadbackExpectation& expected,
    const std::string& idempotencyKey,
    MutationOperationVerificationPolicy policy =
        MutationOperationVerificationPolicy::readbackRequired)
{
    MutationOperation value;
    value.operationId = expected.operationId;
    value.idempotencyKey = idempotencyKey;
    value.actorId = "actor:1";
    value.backendId = expected.backendId;
    value.backendGeneration = expected.backendGeneration;
    value.resourceType = "NativeTimerBinding";
    value.resourceId = expected.nativeTimerBindingId;
    value.expectedRevision = expected.expectedBindingRevision;
    value.actionFamily = "timer.delete";
    value.requestFingerprint = "sha256:" + expected.operationId;
    value.requestedAt = 1000;
    value.deadline = 1800;
    value.verificationPolicy = policy;
    value.state = MutationOperationState::accepted;
    value.updatedAt = value.requestedAt;
    return value;
}

MutationOperation advanceOperation(
    MutationOperationRepository& repository,
    const MutationOperation& candidate,
    MutationOperationState finalState)
{
    auto reserved = repository.reserve(candidate);
    assert(reserved.status == MutationOperationRepositoryStatus::ok);
    auto dispatching = repository.transition(
        candidate.operationId,
        reserved.operation.operationRevision,
        MutationOperationState::accepted,
        MutationOperationState::dispatching,
        "",
        1100);
    assert(dispatching.status == MutationOperationRepositoryStatus::ok);
    auto unresolved = repository.transition(
        candidate.operationId,
        dispatching.operation.operationRevision,
        MutationOperationState::dispatching,
        finalState,
        "native-command:" + candidate.operationId,
        1200);
    assert(unresolved.status == MutationOperationRepositoryStatus::ok);
    return unresolved.operation;
}

NativeTimerBinding markVerified(
    NativeTimerBindingRepository& repository,
    const NativeTimerBinding& created,
    const NativeTimerAbsenceReadbackExpectation& expected,
    const std::string& verifiedOperationId = {})
{
    NativeTimerBinding next = created;
    next.backendGeneration = expected.backendGeneration;
    next.lastObservedAt = 2100;
    next.lastVerifiedOperationId = verifiedOperationId.empty()
        ? expected.operationId
        : verifiedOperationId;
    next.missingSince = 2100;
    next.driftState = NativeTimerBindingDriftState::expectedTransition;
    const auto updated = repository.update(next, created.bindingRevision);
    assert(updated.status == NativeTimerBindingRepositoryStatus::ok);
    return updated.binding;
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

    NativeTimerDeleteOperationCompletionService service(
        operationRepository, bindingRepository);

    const auto created = bindingRepository.create(binding("binding:1", "timer:17"));
    assert(created.ok());
    const auto expected = expectation(created.binding, "operation:delete:1");
    const auto unresolved = advanceOperation(
        operationRepository,
        operation(expected, "idem:delete:1"),
        MutationOperationState::executedUnverified);
    assert(unresolved.state == MutationOperationState::executedUnverified);
    const auto verifiedBinding = markVerified(
        bindingRepository, created.binding, expected);
    assert(verifiedBinding.lastVerifiedOperationId == expected.operationId);

    const auto completed = service.complete(expected, 2200);
    assert(completed.status == NativeTimerDeleteOperationCompletionStatus::completed);
    assert(completed.operation.state == MutationOperationState::succeeded);
    assert(completed.operation.operationRevision == "4");
    assert(completed.operation.resultReference ==
        "native-timer-delete-readback:binding:1:operation:operation:delete:1");

    const auto replay = service.complete(expected, 2300);
    assert(replay.status == NativeTimerDeleteOperationCompletionStatus::alreadyCompleted);
    assert(replay.operation.operationRevision == "4");

    const auto unknownCreated = bindingRepository.create(
        binding("binding:unknown", "timer:unknown"));
    assert(unknownCreated.ok());
    const auto unknownExpected = expectation(
        unknownCreated.binding,
        "operation:delete:unknown",
        NativeTimerReadbackOperationState::outcomeUnknown);
    advanceOperation(
        operationRepository,
        operation(unknownExpected, "idem:delete:unknown"),
        MutationOperationState::outcomeUnknown);
    markVerified(bindingRepository, unknownCreated.binding, unknownExpected);
    const auto reconciled = service.complete(unknownExpected, 2200);
    assert(reconciled.status == NativeTimerDeleteOperationCompletionStatus::completed);
    assert(reconciled.operation.state == MutationOperationState::succeeded);

    auto wrongRevisionExpected = expected;
    wrongRevisionExpected.operationId = "operation:wrong-revision";
    wrongRevisionExpected.expectedBindingRevision = "999";
    auto wrongRevisionOperation = operation(
        wrongRevisionExpected, "idem:wrong-revision");
    wrongRevisionOperation.resourceId = created.binding.nativeTimerBindingId;
    wrongRevisionOperation.expectedRevision = "1";
    advanceOperation(
        operationRepository,
        wrongRevisionOperation,
        MutationOperationState::executedUnverified);
    assert(service.complete(wrongRevisionExpected, 2200).status ==
        NativeTimerDeleteOperationCompletionStatus::identityConflict);

    const auto policyCreated = bindingRepository.create(
        binding("binding:policy", "timer:policy"));
    assert(policyCreated.ok());
    const auto policyExpected = expectation(
        policyCreated.binding, "operation:policy");
    advanceOperation(
        operationRepository,
        operation(
            policyExpected,
            "idem:policy",
            MutationOperationVerificationPolicy::reconciliationRequired),
        MutationOperationState::executedUnverified);
    markVerified(bindingRepository, policyCreated.binding, policyExpected);
    assert(service.complete(policyExpected, 2200).status ==
        NativeTimerDeleteOperationCompletionStatus::verificationPolicyConflict);

    const auto stateCreated = bindingRepository.create(
        binding("binding:state", "timer:state"));
    assert(stateCreated.ok());
    const auto stateExpected = expectation(stateCreated.binding, "operation:state");
    const auto stateReserved = operationRepository.reserve(
        operation(stateExpected, "idem:state"));
    assert(stateReserved.status == MutationOperationRepositoryStatus::ok);
    assert(operationRepository.transition(
        stateExpected.operationId,
        stateReserved.operation.operationRevision,
        MutationOperationState::accepted,
        MutationOperationState::queued,
        "",
        1100).status == MutationOperationRepositoryStatus::ok);
    markVerified(bindingRepository, stateCreated.binding, stateExpected);
    assert(service.complete(stateExpected, 2200).status ==
        NativeTimerDeleteOperationCompletionStatus::operationStateConflict);

    const auto evidenceCreated = bindingRepository.create(
        binding("binding:evidence", "timer:evidence"));
    assert(evidenceCreated.ok());
    const auto evidenceExpected = expectation(
        evidenceCreated.binding, "operation:evidence");
    advanceOperation(
        operationRepository,
        operation(evidenceExpected, "idem:evidence"),
        MutationOperationState::executedUnverified);
    markVerified(
        bindingRepository,
        evidenceCreated.binding,
        evidenceExpected,
        "operation:other");
    assert(service.complete(evidenceExpected, 2200).status ==
        NativeTimerDeleteOperationCompletionStatus::verificationEvidenceMissing);

    auto absentExpected = expected;
    absentExpected.operationId = "operation:not-found";
    assert(service.complete(absentExpected, 2200).status ==
        NativeTimerDeleteOperationCompletionStatus::operationNotFound);

    NativeTimerAbsenceReadbackExpectation noBindingExpected;
    noBindingExpected.operationId = "operation:no-binding";
    noBindingExpected.operationState = NativeTimerReadbackOperationState::executedUnverified;
    noBindingExpected.nativeTimerBindingId = "binding:no-binding";
    noBindingExpected.expectedBindingRevision = "1";
    noBindingExpected.backendId = "backend:a";
    noBindingExpected.backendGeneration = 8;
    noBindingExpected.backendNativeTimerId = "timer:no-binding";
    noBindingExpected.readbackNotBefore = 2000;
    advanceOperation(
        operationRepository,
        operation(noBindingExpected, "idem:no-binding"),
        MutationOperationState::executedUnverified);
    assert(service.complete(noBindingExpected, 2200).status ==
        NativeTimerDeleteOperationCompletionStatus::bindingNotFound);

    const auto bindingMissingCreated = bindingRepository.create(
        binding("binding:missing-operation-target", "timer:missing-operation-target"));
    assert(bindingMissingCreated.ok());
    auto bindingMissingExpected = expectation(
        bindingMissingCreated.binding,
        "operation:binding-not-found");
    advanceOperation(
        operationRepository,
        operation(bindingMissingExpected, "idem:binding-not-found"),
        MutationOperationState::executedUnverified);
    bindingMissingExpected.nativeTimerBindingId = "binding:not-found";
    bindingMissingExpected.expectedBindingRevision = "1";
    auto bindingMissingOperation = operationRepository.findById(
        bindingMissingExpected.operationId);
    assert(bindingMissingOperation.status == MutationOperationRepositoryStatus::ok);
    // The altered expectation no longer matches the durable operation target,
    // so the service fails before any binding lookup can be trusted.
    assert(service.complete(bindingMissingExpected, 2200).status ==
        NativeTimerDeleteOperationCompletionStatus::identityConflict);

    std::cout << "Phase 64 native Timer delete operation completion regression passed\n";
    return 0;
}
