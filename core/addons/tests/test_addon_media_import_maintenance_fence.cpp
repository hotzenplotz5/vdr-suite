#include "AddonMediaImportMaintenanceFence.h"
#include "AccountabilityEventRepository.h"
#include "MutationOperation.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::addons;
using namespace vdrsuite::operations;

namespace {

RequestSecurityContext admin()
{
    RequestSecurityContext actor;
    actor.actor.actorId = "admin-a";
    actor.actor.type = ActorType::User;
    actor.authenticationState = AuthenticationState::Authenticated;
    actor.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    actor.requestId = "freeze-request";
    actor.correlationId = "freeze-correlation";
    actor.grants.push_back({"role.admin", "*"});
    return actor;
}

MutationOperation operation(const std::string& id,
                            const std::string& action,
                            const std::string& backend = "backend-a")
{
    MutationOperation op;
    op.operationId = "op-" + id;
    op.idempotencyKey = "idempotency-" + id;
    op.actorId = "actor-a";
    op.backendId = backend;
    op.backendGeneration = 19;
    op.resourceType = action == "media.import" ?
        "MediaImportSource" : "NativeTimerBinding";
    op.resourceId = action == "media.import" ?
        "source-opaque-1234567890" : "native-timer-1";
    op.expectedRevision = "1";
    op.actionFamily = action;
    op.requestFingerprint = "fingerprint-" + id;
    op.requestedAt = 100;
    op.deadline = 400;
    op.updatedAt = 100;
    op.verificationPolicy = action == "media.import" ?
        MutationOperationVerificationPolicy::reconciliationRequired :
        MutationOperationVerificationPolicy::readbackRequired;
    return op;
}

MutationOperationPayload payload(const MutationOperation& op)
{
    MutationOperationPayload value;
    value.operationId = op.operationId;
    value.payloadType = "addon.media.import";
    value.payloadVersion = 1;
    value.payload = "{\"sourceId\":\"source-opaque-1234567890\"}";
    value.payloadFingerprint = op.requestFingerprint;
    return value;
}

} // namespace

int main()
{
    Database db;
    assert(db.open(":memory:"));
    MutationOperationRepository operations(db);
    AccountabilityEventRepository audit(db);
    AddonMediaImportMaintenanceFence service(db, operations, audit);
    auto actor = admin();

    // Missing schema is not interpreted as an absent (unfrozen) record.
    assert(service.read(actor).status == MaintenanceFenceStatus::unavailable);
    assert(operations.ensureSchema());
    assert(audit.ensureSchema());
    auto state = service.read(actor);
    assert(state.status == MaintenanceFenceStatus::ok &&
           !state.frozen && state.revision == 0);

    auto foreign = actor;
    foreign.grants.clear();
    assert(service.setFrozen(foreign, true, 0,
           {"event-forbidden", "2026-10-22T07:00:00Z"}).status ==
           MaintenanceFenceStatus::forbidden);
    assert(audit.listAll().empty());
    foreign = actor;
    foreign.actor.type = ActorType::Service;
    assert(service.setFrozen(foreign, true, 0,
           {"event-service", "2026-10-22T07:00:00Z"}).status ==
           MaintenanceFenceStatus::forbidden);

    const auto first = operation("first", "media.import");
    const auto firstPayload = payload(first);
    assert(operations.reserveWithPayload(first, firstPayload).ok());
    // A freeze MUST be serialized against all future operation reservations
    // in the same database, independently of the API's earlier capability
    // read, and must span ALL backends of this installed module.
    state = service.setFrozen(
        actor, true, 0, {"event-freeze", "2026-10-22T07:00:00Z"});
    assert(state.status == MaintenanceFenceStatus::ok &&
           state.frozen && state.revision == 1);
    assert(audit.listAll().size() == 1 &&
           audit.listAll()[0].action == "addons.maintenance.freeze");
    // Reading and using the same canonical operation id is not a new write:
    // existing idempotent replay is still queryable while frozen.
    assert(operations.reserveWithPayload(first, firstPayload).status ==
           MutationOperationRepositoryStatus::idempotentReplay);
    const auto second = operation("second", "media.import");
    assert(operations.reserveWithPayload(second, payload(second)).status ==
           MutationOperationRepositoryStatus::maintenanceBlocked);
    const auto otherBackend = operation("third", "media.import", "backend-b");
    assert(operations.reserveWithPayload(otherBackend, payload(otherBackend)).status ==
           MutationOperationRepositoryStatus::maintenanceBlocked);
    assert(operations.findById(second.operationId).status ==
           MutationOperationRepositoryStatus::notFound);
    assert(operations.findPayloadByOperationId(second.operationId).status ==
           MutationOperationRepositoryStatus::notFound);
    // Timer operations remain unchanged by the optional module freeze.
    assert(operations.reserve(operation("timer", "timer.delete")).ok());

    // Reconstructing a Core owner cannot accidentally clear the durable gate.
    AddonMediaImportMaintenanceFence reopened(db, operations, audit);
    assert(reopened.read(actor).frozen);
    assert(reopened.setFrozen(actor, false, 0,
           {"event-stale", "2026-10-22T07:01:00Z"}).status ==
           MaintenanceFenceStatus::revisionConflict);
    assert(reopened.read(actor).revision == 1);
    assert(audit.listAll().size() == 1);
    // Duplicate audit event atomically rolls back an attempted unfreeze.
    assert(reopened.setFrozen(actor, false, 1,
           {"event-freeze", "2026-10-22T07:01:00Z"}).status ==
           MaintenanceFenceStatus::unavailable);
    assert(reopened.read(actor).frozen && reopened.read(actor).revision == 1);
    assert(audit.listAll().size() == 1);
    assert(reopened.setFrozen(actor, false, 1,
           {"event-unfreeze", "2026-10-22T07:01:00Z"}).status ==
           MaintenanceFenceStatus::ok);
    assert(!reopened.read(actor).frozen &&
           reopened.read(actor).revision == 2);
    assert(audit.listAll().size() == 2 &&
           audit.listAll()[1].action == "addons.maintenance.unfreeze");
    assert(operations.reserveWithPayload(second, payload(second)).ok());

    // Different database owners must never be confused for one atomic store.
    Database unrelated;
    assert(unrelated.open(":memory:"));
    AccountabilityEventRepository wrongAudit(unrelated);
    assert(wrongAudit.ensureSchema());
    AddonMediaImportMaintenanceFence wrong(db, operations, wrongAudit);
    assert(wrong.read(actor).status == MaintenanceFenceStatus::forbidden);
    assert(wrong.setFrozen(actor, true, 2,
           {"event-cross-database", "2026-10-22T07:02:00Z"}).status ==
           MaintenanceFenceStatus::forbidden);
    assert(audit.listAll().size() == 2);

    std::cout << "ADDON_MEDIA_IMPORT_MAINTENANCE_FREEZE=PASS\n";
}
