#include "Database.h"
#include "MutationOperation.h"
#include "MutationOperationReadService.h"
#include "MutationOperationRepository.h"

#include <cassert>
#include <string>

using namespace vdrsuite::operations;

namespace
{

MutationOperation operation(
    const std::string& operationId,
    const std::string& actorId)
{
    MutationOperation value;
    value.operationId = operationId;
    value.idempotencyKey = "idem:" + operationId;
    value.actorId = actorId;
    value.backendId = "default";
    value.backendGeneration = 7;
    value.resourceType = "NativeTimerBinding";
    value.resourceId = "binding:" + operationId;
    value.expectedRevision = "4";
    value.expectedResourceFingerprint = "fingerprint:resource";
    value.actionFamily = "timer.delete";
    value.requestFingerprint = "fingerprint:request";
    value.requestedAt = 1000;
    value.deadline = 2000;
    value.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    value.state = MutationOperationState::accepted;
    value.updatedAt = 1000;
    return value;
}

}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    MutationOperationRepository repository(database);
    assert(repository.ensureSchema());

    MutationOperationReadService service(repository);

    const auto reserved =
        repository.reserve(operation("operation:1", "actor:owner"));
    assert(reserved.status == MutationOperationRepositoryStatus::ok);
    assert(reserved.operation.operationRevision == "1");

    const auto owner =
        service.findForActor("operation:1", "actor:owner");
    assert(owner.ok());
    assert(owner.operation.operationId == "operation:1");
    assert(owner.operation.actorId == "actor:owner");
    assert(owner.operation.operationRevision == "1");
    assert(owner.operation.idempotencyKey == "idem:operation:1");

    const auto otherActor =
        service.findForActor("operation:1", "actor:other");
    assert(otherActor.status == MutationOperationReadStatus::notFound);
    assert(otherActor.operation.operationId.empty());

    const auto missing =
        service.findForActor("operation:missing", "actor:owner");
    assert(missing.status == MutationOperationReadStatus::notFound);

    const auto invalidOperation =
        service.findForActor("", "actor:owner");
    assert(invalidOperation.status == MutationOperationReadStatus::invalid);

    const auto invalidActor =
        service.findForActor("operation:1", "");
    assert(invalidActor.status == MutationOperationReadStatus::invalid);

    const auto queued = repository.transition(
        "operation:1",
        "1",
        MutationOperationState::accepted,
        MutationOperationState::queued,
        "",
        1010);
    assert(queued.status == MutationOperationRepositoryStatus::ok);
    assert(queued.operation.operationRevision == "2");

    const auto refreshed =
        service.findForActor("operation:1", "actor:owner");
    assert(refreshed.ok());
    assert(refreshed.operation.state == MutationOperationState::queued);
    assert(refreshed.operation.operationRevision == "2");

    return 0;
}
