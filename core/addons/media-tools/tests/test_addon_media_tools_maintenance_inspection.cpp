#include "AddonMediaToolsMaintenanceInspection.h"
#include "AddonMediaImportMaintenanceFence.h"
#include "AddonMediaImportMaintenanceRead.h"
#include "MediaToolsImportRunLock.h"
#include "MediaImport.h"
#include "AccountabilityEventRepository.h"

#include <cassert>
#include <chrono>
#include <signal.h>
#include <sys/wait.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
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

void testDurableOwnership(const Fixture& f) {
    const auto dbFile = (f.root / "maintenance-owner.sqlite").string();
    Database db1, db2;
    assert(db1.open(dbFile) && db2.open(dbFile));
    MutationOperationRepository operations1(db1), operations2(db2);
    AccountabilityEventRepository audit1(db1), audit2(db2);
    assert(operations1.ensureSchema() && audit1.ensureSchema());
    assert(operations2.ensureSchema() && audit2.ensureSchema());
    AddonMediaImportMaintenanceFence fence1(db1, operations1, audit1);
    AddonMediaImportMaintenanceFence fence2(db2, operations2, audit2);
    auto actor = administrator();
    auto competitor = actor;
    competitor.actor.actorId = "different-admin";
    using Status = MaintenanceFenceStatus;

    assert(!fence1.read(actor).ownershipActive);
    auto shared = vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::sharedExecution);
    assert(shared.has_value());
    assert(!shared->ownsExclusiveLock());
    assert(fence1.claimOwnership(actor, 0, *shared,
        {"owner-with-shared-lock", "2026-10-22T07:15:00Z"}).state.status ==
        Status::forbidden);
    shared.reset();

    auto exclusive = vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::exclusiveMaintenance);
    assert(exclusive && exclusive->ownsExclusiveLock());
    assert(fence1.claimOwnership(
        actor, 1, *exclusive, {"owner-before-freeze", "2026-10-22T07:15:00Z"})
           .state.status == Status::revisionConflict);
    assert(fence1.setFrozen(actor, true, 0,
        {"owner-freeze", "2026-10-22T07:16:00Z"}).status == Status::ok);
    assert(fence1.claimOwnership(actor, 1, *exclusive,
        {"owner-freeze", "2026-10-22T07:16:00Z"}).state.status ==
        Status::unavailable);
    assert(fence1.read(actor).revision == 1 &&
           !fence1.read(actor).ownershipActive);
    auto claim = fence1.claimOwnership(actor, 1, *exclusive,
        {"owner-first-claim", "2026-10-22T07:16:00Z"});
    assert(claim.acquired() && claim.state.revision == 2);
    assert(claim.token.size() == 32);

    // The owner must persist across separate SQLite connections and freshly
    // constructed Core service. Legacy unfreeze by ANY admin is blocked.
    const auto current = fence2.read(competitor);
    assert(current.status == Status::ok && current.frozen &&
           current.ownershipActive && current.revision == 2);
    assert(fence2.setFrozen(competitor, false, 2,
        {"competing-unfreeze", "2026-10-22T07:17:00Z"}).status ==
        Status::ownershipActive);
    assert(fence1.setFrozen(actor, false, 2,
        {"owner-legacy-unfreeze", "2026-10-22T07:18:00Z"}).status ==
        Status::ownershipActive);
    assert(!mediaImportAdmissionUnfrozen(db1));
    assert(!mediaImportAdmissionUnfrozen(db2));
    assert(operations2.reserve(pending()).status ==
           MutationOperationRepositoryStatus::maintenanceBlocked);
    assert(fence2.claimOwnership(competitor, 2, *exclusive,
        {"competing-claim", "2026-10-22T07:19:00Z"}).state.status ==
        Status::ownershipActive);
    assert(fence2.releaseOwnership(competitor, 2, claim.token, *exclusive,
        {"other-admin-release", "2026-10-22T07:20:00Z"}).status ==
        Status::ownershipMismatch);
    assert(fence1.releaseOwnership(actor, 2, std::string(32,'0'), *exclusive,
        {"wrong-token-release", "2026-10-22T07:21:00Z"}).status ==
        Status::ownershipMismatch);
    assert(fence1.releaseOwnership(actor, 1, claim.token, *exclusive,
        {"stale-owner-release", "2026-10-22T07:22:00Z"}).status ==
        Status::revisionConflict);
    assert(fence1.releaseOwnership(actor, 2, claim.token, *exclusive,
        {"owner-first-claim", "2026-10-22T07:23:00Z"}).status ==
        Status::unavailable);
    assert(fence2.read(actor).ownershipActive &&
           fence2.read(actor).revision == 2);
    assert(audit1.listAll().size() == 2);

    const auto released = fence1.releaseOwnership(actor, 2, claim.token,
        *exclusive, {"owner-released", "2026-10-22T07:24:00Z"});
    assert(released.status == Status::ok && released.frozen &&
           !released.ownershipActive && released.revision == 3);
    assert(fence2.read(actor).revision == 3 &&
           !fence2.read(actor).ownershipActive);
    assert(!mediaImportAdmissionUnfrozen(db2));

    // Fresh claim before a crash: no timeout/implicit takeover is allowed.
    auto second = fence1.claimOwnership(actor, 3, *exclusive,
        {"owner-crash-claim", "2026-10-22T07:25:00Z"});
    assert(second.acquired() && second.token != claim.token);
    exclusive.reset();
    assert(fence2.read(actor).ownershipActive);
    assert(fence2.setFrozen(competitor, false, 4,
        {"post-crash-unfreeze", "2026-10-22T07:26:00Z"}).status ==
        Status::ownershipActive);
    assert(operations2.reserve(pending()).status ==
           MutationOperationRepositoryStatus::maintenanceBlocked);
    // The root lock can be acquired after process death, but it does NOT
    // automatically authorize taking over a persistent claimed owner.
    auto another = vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::exclusiveMaintenance);
    assert(another && another->ownsExclusiveLock());
    assert(fence2.claimOwnership(competitor, 4, *another,
        {"attempted-crash-takeover", "2026-10-22T07:27:00Z"}).state.status ==
        Status::ownershipActive);
    assert(!mediaImportAdmissionUnfrozen(db2));
    assert(audit1.listAll().size() == 4);
}

void testOrphanedHevcChildRetainsLock(const Fixture& fixture) {
    using Lock = vdrsuite::mediatools::MediaToolsImportRunLock;
    const fs::path marker = fixture.root / "hevc-child-started.marker";
    // The "Agent" process owns a shared lock and starts one harmless fixed
    // Python child through the real NativeProcessRunner fork/execv path.
    // Killing just that parent must NOT unlock package maintenance while
    // the independent child process is still alive.
    const pid_t agent = ::fork();
    assert(agent >= 0);
    if (agent == 0) {
        auto shared = Lock::tryAcquire(
            fixture.root.string(), Lock::Mode::sharedExecution);
        if (!shared || shared->borrowedDescriptorForChild() < 0)
            ::_exit(91);
        vdrsuite::mediatools::NativeProcessRunner runner(
            std::chrono::seconds(7));
        runner.bindMaintenanceLockFd(shared->borrowedDescriptorForChild());
        const auto result = runner.run({
            "/usr/bin/python3", "-c",
            "import pathlib,sys,time; "
            "pathlib.Path(sys.argv[1]).write_text('ready'); time.sleep(4)",
            marker.string()}, false);
        ::_exit(result.exitCode == 0 ? 0 : 92);
    }
    const auto readyDeadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(3);
    while (!fs::exists(marker) &&
           std::chrono::steady_clock::now() < readyDeadline)
        ::usleep(30000);
    const bool started = fs::exists(marker);
    (void)::kill(agent, SIGKILL);
    int status = 0;
    while (::waitpid(agent, &status, 0) < 0 && errno == EINTR) {}
    assert(started);
    // Only the execv child can still own the lock: its Agent parent is gone.
    assert(!Lock::tryAcquire(
        fixture.root.string(), Lock::Mode::exclusiveMaintenance));
    bool released = false;
    const auto expiry = std::chrono::steady_clock::now() +
        std::chrono::seconds(6);
    while (std::chrono::steady_clock::now() < expiry) {
        auto exclusive = Lock::tryAcquire(
            fixture.root.string(), Lock::Mode::exclusiveMaintenance);
        if (exclusive && exclusive->ownsExclusiveLock()) {
            released = true;
            break;
        }
        ::usleep(40000);
    }
    assert(released);
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

    // A Media Tools package upgrade must not miss concurrent HEVC work.
    // Neither stage nor publish belongs to the import-only freeze, but
    // both MUST deny a supposedly quiescent package snapshot.
    for (const char* action : {"media.shrink.stage", "media.shrink.publish"}) {
        auto shrink = pending();
        const bool stage = std::string(action) == "media.shrink.stage";
        shrink.operationId = stage ? "maintenance-shrink-stage" :
                                     "maintenance-shrink-publish";
        shrink.idempotencyKey = stage ? "maintenance-shrink-stage-idem" :
                                        "maintenance-shrink-publish-idem";
        shrink.actionFamily = action;
        shrink.resourceType = stage ? "Recording" : "MediaShrinkReview";
        shrink.resourceId = stage ? "rec_0123456789abcdef0123456789abcdef" :
                                    "shk_0123456789abcdef0123456789abcdef";
        assert(operations.reserve(shrink).ok());
        assert(inspect().reason == R::operationsUnresolved);
        assert(operations.transition(shrink.operationId, "1",
            MutationOperationState::accepted,
            MutationOperationState::cancelled, "", 101).ok());
        assert(inspect().locallyQuiescent);
    }

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

    // Pending, completed-but-unread and advertised HEVC work must deny the
    // same package maintenance diagnostic as Media Import work.
    for (const char* command : {"addon.media.shrink.stage",
                                "addon.media.shrink.publish"}) {
        const bool stage = std::string(command) == "addon.media.shrink.stage";
        const char* id = stage ? "command-hevc-stage" : "command-hevc-publish";
        assert(db.execute(std::string("INSERT INTO backend_agent_commands VALUES ('") +
                          id + "','" + command + "','assigned');"));
        assert(inspect().reason == R::agentCommandsUnresolved);
        assert(db.execute(std::string("UPDATE backend_agent_commands SET state='completed' ") +
                          "WHERE command_id='" + id + "';"));
        assert(inspect().reason == R::agentResultsMissing);
        assert(db.execute(std::string("INSERT INTO backend_agent_command_results VALUES ('") +
                          id + "');"));
        assert(inspect().locallyQuiescent);
        assert(db.execute(std::string("INSERT INTO backend_agent_command_capabilities ") +
                          "VALUES ('" + command + "');"));
        assert(inspect().reason == R::agentCapabilityStillAdvertised);
        assert(db.execute("DELETE FROM backend_agent_command_capabilities;"));
        assert(inspect().locallyQuiescent);
    }

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

    // A retained inspection MUST keep the same kernel lock until its
    // move-only owner exits, even after ownership changes C++ objects.
    static_assert(!std::is_copy_constructible<MaintenanceInspectionHold>::value,
                  "the maintenance lock may not be duplicated by copying");
    auto held = inspection.inspectAndHold(actor, f.root.string());
    assert(held.holdsExclusiveLock());
    assert(held.inspection.reason == R::locallyQuiescentSnapshot);
    assert(!vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::sharedExecution)
            .has_value());
    assert(inspect().reason == R::runLockUnavailable);
    auto transferred = std::move(held);
    assert(!held.holdsExclusiveLock());
    assert(transferred.holdsExclusiveLock());
    assert(!vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::exclusiveMaintenance)
            .has_value());
    transferred.exclusive.reset();
    assert(!transferred.holdsExclusiveLock());
    assert(inspect().locallyQuiescent);

    // A failed inspection may not leak or return a usable retained lock.
    assert(db.execute("INSERT INTO backend_agent_command_capabilities "
                      "VALUES ('addon.media.import');"));
    auto rejectedHold = inspection.inspectAndHold(actor, f.root.string());
    assert(!rejectedHold.holdsExclusiveLock());
    assert(!rejectedHold.exclusive.has_value());
    assert(rejectedHold.inspection.reason ==
           R::agentCapabilityStillAdvertised);
    assert(vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        f.root.string(),
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::exclusiveMaintenance)
            .has_value());
    assert(db.execute("DELETE FROM backend_agent_command_capabilities;"));

    testOrphanedHevcChildRetainsLock(f);
    testDurableOwnership(f);
    std::cout << "ADDON_MAINTENANCE_OWNER_PERSISTENCE=PASS\n";
    std::cout << "ADDON_GLOBAL_MAINTENANCE_INSPECTION=PASS\n";
    std::cout << "ADDON_MAINTENANCE_EXCLUSIVE_HOLD=PASS\n";
    std::cout << "HEVC_ORPHAN_CHILD_MAINTENANCE_LOCK=PASS\n";
}
