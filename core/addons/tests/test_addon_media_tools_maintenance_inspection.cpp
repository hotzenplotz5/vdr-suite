#include "AddonMediaToolsMaintenanceInspection.h"
#include "AddonMediaImportMaintenanceFence.h"
#include "MediaToolsImportRunLock.h"
#include "AccountabilityEventRepository.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <iostream>

using namespace vdrsuite::addons;
using namespace vdrsuite::operations;
namespace fs = std::filesystem;

namespace {

struct Fixture {
    fs::path root, runLock;
    Fixture() {
        if (::geteuid() != 0 || !std::getenv("TMPDIR") ||
            std::string(std::getenv("TMPDIR")) != "/root")
            throw std::runtime_error("isolated root CI fixture only");
        struct stat parent{};
        if (::stat("/run", &parent) != 0 || !S_ISDIR(parent.st_mode) ||
            parent.st_uid != 0 || (parent.st_mode & 0022) != 0)
            throw std::runtime_error("unsafe test root");
        std::string pattern = "/run/addon-maintenance-inspection-XXXXXX";
        std::vector<char> bytes(pattern.begin(), pattern.end());
        bytes.push_back('\0');
        if (!::mkdtemp(bytes.data())) throw std::runtime_error("mkdtemp");
        root = bytes.data();
        fs::create_directories(root / "etc/vdr-suite/addon-approvals");
        for (const char* d : {"", "etc", "etc/vdr-suite",
                               "etc/vdr-suite/addon-approvals"})
            if (::chmod((root / d).c_str(), 0755) != 0)
                throw std::runtime_error("directory chmod");
        runLock = root / "etc/vdr-suite/addon-approvals" /
            vdrsuite::mediatools::kMediaImportRunLockName;
        { std::ofstream out(runLock, std::ios::trunc | std::ios::binary); }
        if (::chmod(runLock.c_str(), 0644) != 0)
            throw std::runtime_error("runlock chmod");
    }
    ~Fixture() { std::error_code e; fs::remove_all(root,e); }
};

RequestSecurityContext administrator() {
    RequestSecurityContext a;
    a.actor.actorId = "admin";
    a.actor.type = ActorType::User;
    a.authenticationState = AuthenticationState::Authenticated;
    a.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    a.requestId = "maintenance-read";
    a.correlationId = "maintenance-corr";
    a.grants.push_back({"role.admin", "*"});
    return a;
}

MutationOperation pending() {
    MutationOperation op;
    op.operationId = "maintenance-pending-1";
    op.idempotencyKey = "maintenance-idem-1";
    op.actorId = "actor";
    op.backendId = "backend-a";
    op.backendGeneration = 1;
    op.resourceType = "MediaImportSource";
    op.resourceId = "opaque_source_123456";
    op.expectedRevision = "1";
    op.actionFamily = "media.import";
    op.requestFingerprint = "fingerprint";
    op.requestedAt = 100;
    op.deadline = 1000;
    op.updatedAt = 100;
    op.verificationPolicy = MutationOperationVerificationPolicy::reconciliationRequired;
    return op;
}

}

int main() {
    Fixture f;
    Database db;
    assert(db.open(":memory:"));
    AddonActivationIntentRepository intents(db);
    MutationOperationRepository operations(db);
    AccountabilityEventRepository audit(db);
    AddonMediaToolsMaintenanceInspection inspection(db, intents, operations);
    AddonMediaImportMaintenanceFence fence(db, operations, audit);
    auto actor = administrator();
    auto inspect = [&]() { return inspection.inspect(actor, f.root.string()); };
    using R = MaintenanceInspectionReason;
    assert(inspect().reason == R::storageUnavailable);

    assert(intents.ensureSchema());
    assert(operations.ensureSchema());
    assert(audit.ensureSchema());
    // The real command owner uses these same table/column names. Synthetic
    // Agent rows isolate the inspection SQL without executing a real Agent.
    assert(db.execute("CREATE TABLE backend_agent_commands("
                      "command_id TEXT PRIMARY KEY,command_type TEXT NOT NULL,"
                      "state TEXT NOT NULL);"));
    assert(db.execute("CREATE TABLE backend_agent_command_results("
                      "command_id TEXT PRIMARY KEY);"));
    assert(db.execute("CREATE TABLE backend_agent_command_capabilities("
                      "command_type TEXT NOT NULL);"));
    assert(inspect().reason == R::globalFreezeRequired);

    auto unauthorized = actor;
    unauthorized.grants.clear();
    assert(inspection.inspect(unauthorized,f.root.string()).reason ==
           R::forbidden);
    Database other;
    assert(other.open(":memory:"));
    AddonActivationIntentRepository unrelated(other);
    AddonMediaToolsMaintenanceInspection wrong(db, unrelated, operations);
    assert(wrong.inspect(actor,f.root.string()).reason ==
           R::incompatibleDatabases);

    const auto accepted = operations.reserve(pending());
    assert(accepted.ok());
    assert(fence.setFrozen(actor, true, 0,
        {"maintenance-freeze-event", "2026-10-22T07:00:00Z"}).status ==
        MaintenanceFenceStatus::ok);
    assert(inspect().reason == R::auditedDisableMissing);
    assert(intents.update(actor, {"rectools","backend-a",false,0},
                          {}, {}).status == IntentStatus::ok);
    assert(inspect().reason == R::operationsUnresolved);
    assert(operations.transition("maintenance-pending-1", "1",
        MutationOperationState::accepted,
        MutationOperationState::cancelled, "",101).ok());
    assert(inspect().locallyQuiescent);

    BackendAccessDecision writable;
    writable.backendId="backend-b";
    writable.backendFound=true;
    writable.allowed=true;
    RuntimeEvidence evidence;
    evidence.moduleId="rectools";
    evidence.packageName="vdr-suite-addon-media-tools";
    evidence.backendId="backend-b";
    evidence.installed=evidence.manifestValid=evidence.packageTrusted=true;
    evidence.versionCompatible=evidence.handlerRegistered=
        evidence.handlerHealthy=true;
    assert(intents.update(actor, {"rectools","backend-b",true,0},
                          writable,evidence).status == IntentStatus::ok);
    assert(inspect().reason == R::activationStillEnabled);
    assert(intents.update(actor, {"rectools","backend-b",false,1},
                          {},{}).status == IntentStatus::ok);

    assert(db.execute("INSERT INTO backend_agent_commands VALUES "
                      "('command-1','addon.media.import','assigned');"));
    assert(inspect().reason == R::agentCommandsUnresolved);
    assert(db.execute("UPDATE backend_agent_commands SET state='completed' "
                      "WHERE command_id='command-1';"));
    assert(inspect().reason == R::agentResultsMissing);
    assert(db.execute("INSERT INTO backend_agent_command_results "
                      "VALUES ('command-1');"));
    assert(inspect().locallyQuiescent);
    assert(db.execute("INSERT INTO backend_agent_command_capabilities "
                      "VALUES ('addon.media.import');"));
    assert(inspect().reason == R::agentCapabilityStillAdvertised);
    assert(db.execute("DELETE FROM backend_agent_command_capabilities;"));

    auto shared = vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::sharedExecution);
    assert(shared.has_value());
    assert(inspect().reason == R::runLockUnavailable);
    shared.reset();
    assert(inspect().locallyQuiescent);

    // A diagnostic cannot clear a freeze, grant an install, or retain an
    // exclusive lock: verify no persistent state was changed by inspection.
    assert(fence.read(actor).frozen);
    assert(vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::exclusiveMaintenance)
            .has_value());
    assert(audit.listAll().size() == 1);
    std::cout << "ADDON_GLOBAL_MAINTENANCE_INSPECTION=PASS\n";
}
