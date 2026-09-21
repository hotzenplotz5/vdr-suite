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
    operation.expectedResourceFingerprint =
        "native-timer-observed-state/1|3:abc|";
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

MutationOperationPayload makePayload(
    const std::string& operationId = "op-payload",
    const std::string& body = "payload-v1",
    const std::string& fingerprint = "payload-fingerprint-v1")
{
    MutationOperationPayload payload;
    payload.operationId = operationId;
    payload.payloadType = "test.operation.payload";
    payload.payloadVersion = 1;
    payload.payload = body;
    payload.payloadFingerprint = fingerprint;
    return payload;
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

void assertLegacySchemaMigration()
{
    Database database;
    assert(database.open(":memory:"));
    assert(database.execute(
        "CREATE TABLE mutation_operations ("
        "operation_id TEXT PRIMARY KEY,"
        "operation_revision INTEGER NOT NULL,"
        "idempotency_key TEXT NOT NULL,actor_id TEXT NOT NULL,backend_id TEXT NOT NULL,"
        "backend_generation INTEGER NOT NULL,resource_type TEXT NOT NULL,resource_id TEXT NOT NULL,"
        "expected_revision TEXT NOT NULL,action_family TEXT NOT NULL,request_fingerprint TEXT NOT NULL,"
        "requested_at INTEGER NOT NULL,deadline INTEGER NOT NULL,verification_policy TEXT NOT NULL,"
        "state TEXT NOT NULL,result_reference TEXT NOT NULL,updated_at INTEGER NOT NULL,"
        "CHECK(operation_revision>0),CHECK(backend_generation>0),"
        "CHECK(verification_policy IN ('none','readback_required','event_confirmation','reconciliation_required')),"
        "CHECK(state IN ('accepted','rejected','conflict','queued','dispatching','executed_unverified','succeeded','failed_before_dispatch','failed_verified','outcome_unknown','cancelled'))"
        ");"));

    MutationOperationRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.tableExists("mutation_operation_payloads"));

    auto operation = makeOperation(
        "op-migrated", "idem-migrated", "sha256:req-migrated");
    operation.resourceId = "binding-migrated";
    const auto reserved = repository.reserve(operation);
    assert(reserved.status == MutationOperationRepositoryStatus::ok);
    assert(reserved.operation.expectedResourceFingerprint ==
        operation.expectedResourceFingerprint);

    const auto found = repository.findById("op-migrated");
    assert(found.status == MutationOperationRepositoryStatus::ok);
    assert(found.operation.expectedResourceFingerprint ==
        operation.expectedResourceFingerprint);
    assert(repository.findPayloadByOperationId("op-migrated").status ==
        MutationOperationRepositoryStatus::notFound);
}

void assertAtomicPayloadReservation()
{
    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository repository(database);
    assert(repository.ensureSchema());

    auto operation = makeOperation(
        "op-payload", "idem-payload", "sha256:req-payload");
    operation.resourceType = "TimerAssignment";
    operation.resourceId = "assignment-1";
    operation.expectedRevision = "4";
    operation.expectedResourceFingerprint =
        "native-timer-specification/1|desired";
    operation.actionFamily = "timer.create";

    const auto payload = makePayload();
    const auto reserved = repository.reserveWithPayload(operation, payload);
    assertState(
        reserved,
        MutationOperationRepositoryStatus::ok,
        MutationOperationState::accepted,
        "1");

    const auto foundPayload = repository.findPayloadByOperationId("op-payload");
    assert(foundPayload.ok());
    assert(foundPayload.payload.operationId == "op-payload");
    assert(foundPayload.payload.payloadType == "test.operation.payload");
    assert(foundPayload.payload.payloadVersion == 1);
    assert(foundPayload.payload.payload == "payload-v1");
    assert(foundPayload.payload.payloadFingerprint ==
        "payload-fingerprint-v1");

    const auto replay = repository.reserveWithPayload(operation, payload);
    assertState(
        replay,
        MutationOperationRepositoryStatus::idempotentReplay,
        MutationOperationState::accepted,
        "1");

    auto changedBody = payload;
    changedBody.payload = "payload-v1-changed";
    assert(repository.reserveWithPayload(operation, changedBody).status ==
        MutationOperationRepositoryStatus::operationConflict);

    auto changedFingerprint = payload;
    changedFingerprint.payloadFingerprint = "payload-fingerprint-other";
    assert(repository.reserveWithPayload(operation, changedFingerprint).status ==
        MutationOperationRepositoryStatus::operationConflict);

    auto changedVersion = payload;
    changedVersion.payloadVersion = 2;
    assert(repository.reserveWithPayload(operation, changedVersion).status ==
        MutationOperationRepositoryStatus::operationConflict);

    auto wrongOperation = payload;
    wrongOperation.operationId = "op-other";
    assert(repository.reserveWithPayload(operation, wrongOperation).status ==
        MutationOperationRepositoryStatus::invalid);

    auto emptyPayload = payload;
    emptyPayload.payload.clear();
    assert(repository.reserveWithPayload(
        makeOperation("op-empty", "idem-empty", "sha256:req-empty"),
        emptyPayload).status == MutationOperationRepositoryStatus::invalid);

    auto oversized = makePayload(
        "op-oversized", std::string(64 * 1024 + 1, 'x'), "fingerprint");
    auto oversizedOperation = makeOperation(
        "op-oversized", "idem-oversized", "sha256:req-oversized");
    oversizedOperation.resourceId = "binding-oversized";
    assert(repository.reserveWithPayload(oversizedOperation, oversized).status ==
        MutationOperationRepositoryStatus::invalid);

    auto legacyOperation = makeOperation(
        "op-no-payload", "idem-no-payload", "sha256:req-no-payload");
    legacyOperation.resourceId = "binding-no-payload";
    assert(repository.reserve(legacyOperation).status ==
        MutationOperationRepositoryStatus::ok);
    assert(repository.findPayloadByOperationId("op-no-payload").status ==
        MutationOperationRepositoryStatus::notFound);

    auto latePayload = makePayload("op-no-payload");
    assert(repository.reserveWithPayload(legacyOperation, latePayload).status ==
        MutationOperationRepositoryStatus::operationConflict);
    assert(repository.findPayloadByOperationId("op-no-payload").status ==
        MutationOperationRepositoryStatus::notFound);
}
}

int main()
{
    assertLegacySchemaMigration();
    assertAtomicPayloadReservation();

    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.tableExists("mutation_operations"));
    assert(database.tableExists("mutation_operation_payloads"));

    const auto reserved = repository.reserve(makeOperation());
    assertState(
        reserved,
        MutationOperationRepositoryStatus::ok,
        MutationOperationState::accepted,
        "1");
    assert(reserved.operation.expectedResourceFingerprint ==
        "native-timer-observed-state/1|3:abc|");

    const auto replay = repository.reserve(makeOperation());
    assertState(
        replay,
        MutationOperationRepositoryStatus::idempotentReplay,
        MutationOperationState::accepted,
        "1");
    assert(replay.operation.expectedResourceFingerprint ==
        reserved.operation.expectedResourceFingerprint);

    auto changedExpectedResource = makeOperation();
    changedExpectedResource.expectedResourceFingerprint =
        "native-timer-observed-state/1|3:def|";
    const auto resourceFingerprintConflict =
        repository.reserve(changedExpectedResource);
    assert(resourceFingerprintConflict.status ==
        MutationOperationRepositoryStatus::operationConflict);

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
    assert(foundScope.operation.expectedResourceFingerprint ==
        reserved.operation.expectedResourceFingerprint);

    auto queued = repository.transition(
        "op-1", "1", MutationOperationState::accepted,
        MutationOperationState::queued, "", 1010);
    assertState(queued, MutationOperationRepositoryStatus::ok,
        MutationOperationState::queued, "2");
    assert(queued.operation.expectedResourceFingerprint ==
        reserved.operation.expectedResourceFingerprint);

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
