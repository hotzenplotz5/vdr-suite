#include "Database.h"
#include "MutationOperation.h"
#include "MutationOperationRepository.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::operations;

namespace
{
MutationOperation makeOperation(
    const std::string& operationId = "op-1",
    const std::string& idempotencyKey = "idem-1",
    const std::string& fingerprint = "sha256:req-1")
{
    MutationOperation operation;
    operation.operationId = operationId;
    operation.idempotencyKey = idempotencyKey;
    operation.actorId = "actor-1";
    operation.backendId = "backend-1";
    operation.backendGeneration = 7;
    operation.resourceType = "NativeTimerBinding";
    operation.resourceId = "binding-1";
    operation.expectedRevision = "19";
    operation.actionFamily = "timer.delete";
    operation.requestFingerprint = fingerprint;
    operation.requestedAt = 1000;
    operation.deadline = 2000;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    operation.state = MutationOperationState::accepted;
    operation.updatedAt = operation.requestedAt;
    return operation;
}

void assertState(
    const MutationOperationRepositoryResult& result,
    MutationOperationRepositoryStatus status,
    MutationOperationState state,
    const std::string& revision)
{
    assert(result.status == status);
    assert(result.operation.state == state);
    assert(result.operation.operationRevision == revision);
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.tableExists("mutation_operations"));

    const auto reserved = repository.reserve(makeOperation());
    assertState(
        reserved,
        MutationOperationRepositoryStatus::ok,
        MutationOperationState::accepted,
        "1");

    const auto replay = repository.reserve(makeOperation());
    assertState(
        replay,
        MutationOperationRepositoryStatus::idempotentReplay,
        MutationOperationState::accepted,
        "1");

    auto conflictingFingerprint = makeOperation("op-2", "idem-1", "sha256:req-2");
    const auto idempotencyConflict = repository.reserve(conflictingFingerprint);
    assert(idempotencyConflict.status ==
        MutationOperationRepositoryStatus::idempotencyConflict);
    assert(idempotencyConflict.operation.operationId == "op-1");

    auto operationCollision = makeOperation("op-1", "idem-2", "sha256:req-3");
    operationCollision.resourceId = "binding-2";
    const auto operationConflict = repository.reserve(operationCollision);
    assert(operationConflict.status ==
        MutationOperationRepositoryStatus::operationConflict);

    auto differentActor = makeOperation("op-actor-2", "idem-1", "sha256:req-4");
    differentActor.actorId = "actor-2";
    const auto isolatedScope = repository.reserve(differentActor);
    assert(isolatedScope.status == MutationOperationRepositoryStatus::ok);

    const auto foundScope = repository.findByIdempotencyScope(
        "actor-1", "backend-1", "NativeTimerBinding", "binding-1",
        "timer.delete", "idem-1");
    assert(foundScope.status == MutationOperationRepositoryStatus::ok);
    assert(foundScope.operation.operationId == "op-1");

    auto queued = repository.transition(
        "op-1", "1", MutationOperationState::accepted,
        MutationOperationState::queued, "", 1010);
    assertState(queued, MutationOperationRepositoryStatus::ok,
        MutationOperationState::queued, "2");

    const auto queuedReplay = repository.transition(
        "op-1", "1", MutationOperationState::accepted,
        MutationOperationState::queued, "", 1010);
    assertState(queuedReplay,
        MutationOperationRepositoryStatus::idempotentReplay,
        MutationOperationState::queued, "2");

    const auto staleRevision = repository.transition(
        "op-1", "1", MutationOperationState::queued,
        MutationOperationState::dispatching, "", 1020);
    assert(staleRevision.status ==
        MutationOperationRepositoryStatus::revisionConflict);

    auto dispatching = repository.transition(
        "op-1", "2", MutationOperationState::queued,
        MutationOperationState::dispatching, "", 1020);
    assertState(dispatching, MutationOperationRepositoryStatus::ok,
        MutationOperationState::dispatching, "3");

    auto executed = repository.transition(
        "op-1", "3", MutationOperationState::dispatching,
        MutationOperationState::executedUnverified,
        "native-receipt:cmd-1", 1030);
    assertState(executed, MutationOperationRepositoryStatus::ok,
        MutationOperationState::executedUnverified, "4");

    auto succeeded = repository.transition(
        "op-1", "4", MutationOperationState::executedUnverified,
        MutationOperationState::succeeded,
        "native-timer-binding:binding-1@readback", 1040);
    assertState(succeeded, MutationOperationRepositoryStatus::ok,
        MutationOperationState::succeeded, "5");

    const auto terminalReject = repository.transition(
        "op-1", "5", MutationOperationState::succeeded,
        MutationOperationState::failedVerified, "failure", 1050);
    assert(terminalReject.status ==
        MutationOperationRepositoryStatus::stateConflict);

    auto unknown = makeOperation("op-unknown", "idem-unknown", "sha256:req-u");
    unknown.resourceId = "binding-u";
    assert(repository.reserve(unknown).status == MutationOperationRepositoryStatus::ok);
    assert(repository.transition(
        "op-unknown", "1", MutationOperationState::accepted,
        MutationOperationState::dispatching, "", 1010).status ==
        MutationOperationRepositoryStatus::ok);
    assert(repository.transition(
        "op-unknown", "2", MutationOperationState::dispatching,
        MutationOperationState::outcomeUnknown, "transport-timeout", 1020).status ==
        MutationOperationRepositoryStatus::ok);
    const auto reconciled = repository.transition(
        "op-unknown", "3", MutationOperationState::outcomeUnknown,
        MutationOperationState::succeeded, "authoritative-readback", 1030);
    assertState(reconciled, MutationOperationRepositoryStatus::ok,
        MutationOperationState::succeeded, "4");

    auto invalidJump = makeOperation("op-invalid-jump", "idem-invalid-jump", "sha256:req-j");
    invalidJump.resourceId = "binding-j";
    assert(repository.reserve(invalidJump).status == MutationOperationRepositoryStatus::ok);
    const auto jump = repository.transition(
        "op-invalid-jump", "1", MutationOperationState::accepted,
        MutationOperationState::succeeded, "bad", 1010);
    assert(jump.status == MutationOperationRepositoryStatus::stateConflict);

    auto invalid = makeOperation("op-invalid", "idem-invalid", "sha256:req-i");
    invalid.backendGeneration = 0;
    assert(repository.reserve(invalid).status == MutationOperationRepositoryStatus::invalid);

    std::cout << "Phase 64 mutation operation repository regression passed\n";
    return 0;
}
