#include "AddonMediaToolsMaintenanceReadService.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::addons;
using namespace vdrsuite::operations;

namespace {
RequestSecurityContext administrator()
{
    RequestSecurityContext actor;
    actor.actor.actorId = "admin";
    actor.actor.type = ActorType::User;
    actor.authenticationState = AuthenticationState::Authenticated;
    actor.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    actor.grants.push_back({"role.admin", "*"});
    return actor;
}

MutationOperation mediaImport(const std::string& id,
                              const std::string& backend = "backend-a")
{
    MutationOperation operation;
    operation.operationId = "operation-" + id;
    operation.idempotencyKey = "key-" + id;
    operation.actorId = "import-user";
    operation.backendId = backend;
    operation.backendGeneration = 19;
    operation.resourceType = "MediaImportSource";
    operation.resourceId = "inventory_source_token_123";
    operation.expectedRevision = "1";
    operation.actionFamily = "media.import";
    operation.requestFingerprint = "fingerprint-" + id;
    operation.requestedAt = 100;
    operation.deadline = 500;
    operation.updatedAt = 100;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::reconciliationRequired;
    return operation;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    AddonActivationIntentRepository activation(database);
    MutationOperationRepository operations(database);
    AddonMediaToolsMaintenanceReadService service(activation, operations);
    auto actor = administrator();
    auto read = [&](const std::string& backend = "backend-a") {
        return service.inspect(actor, backend);
    };
    auto result = read();
    assert(!result.locallyQuiescent &&
           result.reason == MaintenanceReadReason::activationUnavailable);
    actor.grants.clear();
    assert(read().reason == MaintenanceReadReason::forbidden);
    actor = administrator();
    actor.actor.type = ActorType::Service;
    assert(read().reason == MaintenanceReadReason::forbidden);
    actor = administrator();
    assert(read("../unsafe").reason == MaintenanceReadReason::forbidden);

    assert(activation.ensureSchema());
    result = read();
    // Even a default-disabled missing row is not an audited disable.
    assert(result.reason ==
           MaintenanceReadReason::activationNotExplicitlyDisabled);
    assert(!result.locallyQuiescent && result.observedIntentRevision == 0);
    assert(activation.update(actor, {"rectools", "backend-a", false, 0},
                             {}, {}).status == IntentStatus::ok);
    result = read();
    assert(result.reason == MaintenanceReadReason::operationsUnavailable);
    assert(operations.ensureSchema());
    result = read();
    assert(result.locallyQuiescent &&
           result.reason == MaintenanceReadReason::locallyQuiescentSnapshot &&
           result.observedIntentRevision == 1);

    // A second backend has an independent activation history.
    assert(read("backend-b").reason ==
           MaintenanceReadReason::activationNotExplicitlyDisabled);
    assert(activation.update(actor, {"rectools", "backend-b", false, 0},
                             {}, {}).status == IntentStatus::ok);

    // An import accepted before maintenance must block package changes.
    auto first = operations.reserve(mediaImport("first"));
    assert(first.ok());
    result = read();
    assert(result.reason == MaintenanceReadReason::unresolvedMediaImports &&
           result.unresolvedOperations == 1);
    assert(read("backend-b").locallyQuiescent);
    // A rejected or cancelled operation can no longer write a recording.
    auto cancelled = operations.transition("operation-first", "1",
        MutationOperationState::accepted, MutationOperationState::cancelled,
        "", 101);
    assert(cancelled.ok());
    assert(read().locallyQuiescent);

    // An uncertain result is NOT a terminal failure and needs independent
    // VDR recording readback before handler replacement is considered.
    assert(operations.reserve(mediaImport("second")).ok());
    auto queued = operations.transition("operation-second", "1",
        MutationOperationState::accepted, MutationOperationState::queued,
        "", 101);
    assert(queued.ok());
    auto dispatch = operations.transition("operation-second", "2",
        MutationOperationState::queued, MutationOperationState::dispatching,
        "", 102);
    assert(dispatch.ok());
    auto executed = operations.transition("operation-second", "3",
        MutationOperationState::dispatching,
        MutationOperationState::executedUnverified, "", 103);
    assert(executed.ok());
    assert(read().reason == MaintenanceReadReason::unresolvedMediaImports);
    auto unknown = operations.transition("operation-second", "4",
        MutationOperationState::executedUnverified,
        MutationOperationState::outcomeUnknown, "", 104);
    assert(unknown.ok());
    assert(read().reason == MaintenanceReadReason::unresolvedMediaImports);
    auto reconciled = operations.transition("operation-second", "5",
        MutationOperationState::outcomeUnknown,
        MutationOperationState::succeeded, "native-recording-id", 105);
    assert(reconciled.ok());
    assert(read().locallyQuiescent);

    // The maintenance reader does not activate or disable anything. An
    // enabled intent (even with no open jobs) always blocks the preview.
    BackendAccessDecision writable;
    writable.backendId = "backend-a";
    writable.backendFound = true;
    writable.allowed = true;
    RuntimeEvidence trusted;
    trusted.moduleId = "rectools";
    trusted.packageName = "vdr-suite-addon-media-tools";
    trusted.backendId = "backend-a";
    trusted.agentId = "maintenance-agent";
    trusted.agentInstanceId = "maintenance-instance";
    trusted.backendGeneration = 19;
    trusted.approvedHandlerSha256 = std::string(64, 'a');
    trusted.installed = trusted.manifestValid = trusted.packageTrusted = true;
    trusted.versionCompatible = trusted.handlerRegistered =
        trusted.handlerHealthy = true;
    assert(activation.update(actor, {"rectools", "backend-a", true, 1},
                             writable, trusted).status == IntentStatus::ok);
    assert(read().reason ==
           MaintenanceReadReason::activationNotExplicitlyDisabled);
    assert(activation.update(actor, {"rectools", "backend-a", false, 2},
                             {}, {}).status == IntentStatus::ok);
    assert(read().locallyQuiescent);
    assert(read().observedIntentRevision == 3);

    // The bounded global operation query can omit a backend under load.
    // It must never return a spurious clean result when saturated.
    for (int i = 0; i < 256; ++i) {
        const auto item = mediaImport("bulk-" + std::to_string(i), "backend-b");
        assert(operations.reserve(item).ok());
    }
    result = read();
    assert(!result.locallyQuiescent &&
           result.reason == MaintenanceReadReason::scanSaturated &&
           result.unresolvedOperations == 256);

    std::cout << "ADDON_MAINTENANCE_READ=PASS\n";
}
