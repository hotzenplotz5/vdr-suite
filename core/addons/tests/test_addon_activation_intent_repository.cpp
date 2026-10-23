#include "AddonActivationIntentRepository.h"
#include <sqlite3.h>

#include <atomic>
#include <cassert>
#include <thread>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace vdrsuite::addons;

struct Fixture {
    fs::path folder;
    Database database;
    Fixture()
    {
        const char* temp = std::getenv("TMPDIR");
        if (!temp || !*temp) throw std::runtime_error("TMPDIR must be configured for isolated test");
        std::string pattern = (fs::path(temp) / "suite-addon-intent-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        const char* created = mkdtemp(buffer.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        folder = created;
        if (!database.open((folder / "state.sqlite").string()))
            throw std::runtime_error("cannot open isolated DB");
    }
    ~Fixture() { database.close(); std::error_code ec; fs::remove_all(folder, ec); }
};

RequestSecurityContext admin()
{
    RequestSecurityContext user;
    user.actor.actorId = "administrator";
    user.actor.type = ActorType::User;
    user.authenticationState = AuthenticationState::Authenticated;
    user.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    user.grants.push_back({"role.admin", "*"});
    return user;
}

BackendAccessDecision backend()
{
    BackendAccessDecision b;
    b.backendId = "backend-a";
    b.backendFound = true;
    b.allowed = true;
    b.readOnly = false;
    return b;
}

RuntimeEvidence trustedEvidence()
{
    RuntimeEvidence e;
    e.moduleId = "rectools";
    e.packageName = "vdr-suite-addon-media-tools";
    e.backendId = "backend-a";
    e.installed = true;
    e.manifestValid = true;
    e.packageTrusted = true;
    e.versionCompatible = true;
    e.handlerRegistered = true;
    e.handlerHealthy = true;
    return e;
}

int main()
{
    int checks = 0;
    Fixture f;
    AddonActivationIntentRepository store(f.database);
    auto actor = admin();
    auto b = backend();
    auto evidence = trustedEvidence();

    auto read = store.read(actor, "rectools", "backend-a");
    assert(read.status == IntentStatus::unavailable);
    ++checks;

    assert(store.ensureSchema());
    assert(store.ensureSchema());
    // Without the canonical operations maintenance schema, enable MUST
    // fail closed; no missing-table shortcut may grant an administrator wish.
    assert(store.update(actor, {"rectools","backend-a",true,0},
                        b, evidence).status == IntentStatus::unavailable);
    assert(f.database.execute(
        "CREATE TABLE addon_media_import_maintenance ("
        "module_id TEXT PRIMARY KEY CHECK(module_id='rectools'),"
        "frozen INTEGER NOT NULL CHECK(frozen IN(0,1)),"
        "revision INTEGER NOT NULL CHECK(revision>0),"
        "updated_by_actor_id TEXT NOT NULL);"));
    read = store.read(actor, "rectools", "backend-a");
    assert(read.status == IntentStatus::ok && read.intent.revision == 0 &&
           !read.intent.desiredEnabled);
    ++checks;

    IntentMutation enable{"rectools", "backend-a", true, 0};
    auto denied = actor;
    denied.grants.clear();
    assert(store.update(denied, enable, b, evidence).status == IntentStatus::forbidden);
    assert(store.read(denied, "rectools", "backend-a").status == IntentStatus::forbidden);
    ++checks;

    denied = actor;
    denied.permissionGrantResolution = PermissionGrantResolutionState::Unavailable;
    assert(store.update(denied, enable, b, evidence).status == IntentStatus::forbidden);
    ++checks;

    evidence.packageTrusted = false;
    assert(store.update(actor, enable, b, evidence).status == IntentStatus::ineligible);
    ++checks;
    evidence = trustedEvidence();
    evidence.handlerRegistered = false;
    assert(store.update(actor, enable, b, evidence).status == IntentStatus::ineligible);
    ++checks;
    evidence = trustedEvidence();
    b.readOnly = true;
    b.allowed = false;
    assert(store.update(actor, enable, b, evidence).status == IntentStatus::ineligible);
    ++checks;
    b = backend();
    assert(store.update(actor, {"unknown", "backend-a", true, 0}, b, evidence).status ==
           IntentStatus::invalid);
    assert(store.update(actor, {"rectools", "../bad", true, 0}, b, evidence).status ==
           IntentStatus::invalid);
    ++checks;

    auto committed = store.update(actor, enable, b, evidence);
    assert(committed.status == IntentStatus::ok);
    assert(committed.intent.desiredEnabled && committed.intent.revision == 1);
    ++checks;

    // A current-revision repeated intent is a durable no-op, not a second
    // update. A stale revision is still a conflict even if the bit matches.
    auto replay = store.update(actor, {"rectools", "backend-a", true, 1}, b, evidence);
    assert(replay.status == IntentStatus::ok &&
           replay.intent.desiredEnabled && replay.intent.revision == 1);
    ++checks;
    // The maintenance owner may use a different SQLite connection. The
    // administrator enable CAS must observe its committed global freeze.
    Database maintenanceOwner;
    assert(maintenanceOwner.open((f.folder / "state.sqlite").string()));
    assert(maintenanceOwner.execute(
        "INSERT INTO addon_media_import_maintenance "
        "VALUES ('rectools',1,1,'maintenance-admin');"));
    assert(store.update(actor, {"rectools","backend-a",true,1},
                        b, evidence).status == IntentStatus::ineligible);
    assert(store.read(actor,"rectools","backend-a").intent.revision == 1);
    assert(maintenanceOwner.execute(
        "DELETE FROM addon_media_import_maintenance;"));
    maintenanceOwner.close();
    ++checks;
    auto stale = store.update(actor, enable, b, evidence);
    assert(stale.status == IntentStatus::revisionConflict);
    ++checks;

    // Desired intent persists across separately constructed repository owners.
    AddonActivationIntentRepository afterRestart(f.database);
    read = afterRestart.read(actor, "rectools", "backend-a");
    assert(read.status == IntentStatus::ok && read.intent.desiredEnabled &&
           read.intent.revision == 1);
    ++checks;

    // Disable must work even when the handler is gone or backend is offline;
    // storing disabled requires admin authority and a correct revision.
    evidence = {};
    b = {};
    auto disable = afterRestart.update(actor, {"rectools", "backend-a", false, 1}, b, evidence);
    assert(disable.status == IntentStatus::ok && !disable.intent.desiredEnabled &&
           disable.intent.revision == 2);
    ++checks;

    // Backend-specific state: no cross-backend enabling or revision leakage.
    read = afterRestart.read(actor, "rectools", "backend-b");
    assert(read.status == IntentStatus::ok && read.intent.revision == 0);
    ++checks;

    denied = admin();
    denied.actor.active = false;
    assert(store.update(denied, {"rectools", "backend-a", false, 2}, b, evidence).status ==
           IntentStatus::forbidden);
    ++checks;

    // Inactive photo/music scaffolds cannot express enabled intent.
    b = backend();
    evidence = trustedEvidence();
    for (auto module : {"image", "music"}) {
        assert(store.update(actor, {module, "backend-a", true, 0}, b, evidence).status ==
               IntentStatus::ineligible);
    }
    ++checks;

    // TVScraper is native SuiteBridge/Core metadata integration, not an add-on.
    assert(store.read(actor, "tvscraper", "backend-a").status == IntentStatus::invalid);
    assert(store.update(actor, {"tvscraper", "backend-a", false, 0}, b, evidence).status ==
           IntentStatus::invalid);
    assert(store.update(actor, {"tvscraper", "backend-a", true, 0}, b, evidence).status ==
           IntentStatus::invalid);
    ++checks;

    read = store.read(actor, "rectools", "backend-a");
    assert(read.status == IntentStatus::ok && !read.intent.desiredEnabled &&
           read.intent.revision == 2);
    ++checks;


    // Competing connections against one SQLite file: a revision must never
    // be accepted twice, regardless of which thread commits first.
    Database otherDatabase;
    assert(otherDatabase.open((f.folder / "state.sqlite").string()));
    AddonActivationIntentRepository concurrent(otherDatabase);
    std::atomic<bool> start{false};
    IntentResult resultA;
    IntentResult resultB;
    std::thread first([&] {
        while (!start.load()) std::this_thread::yield();
        resultA = store.update(actor, {"rectools", "backend-a", true, 2}, b, evidence);
    });
    std::thread second([&] {
        while (!start.load()) std::this_thread::yield();
        resultB = concurrent.update(actor, {"rectools", "backend-a", true, 2}, b, evidence);
    });
    start.store(true);
    first.join();
    second.join();
    const int successes = static_cast<int>(resultA.status == IntentStatus::ok) +
        static_cast<int>(resultB.status == IntentStatus::ok);
    const int conflicts = static_cast<int>(resultA.status == IntentStatus::revisionConflict) +
        static_cast<int>(resultB.status == IntentStatus::revisionConflict);
    assert(successes == 1 && conflicts == 1);
    read = store.read(actor, "rectools", "backend-a");
    assert(read.status == IntentStatus::ok && read.intent.revision == 3 &&
           read.intent.desiredEnabled);
    otherDatabase.close();
    ++checks;

    // Atomic audit append: failed accountability persistence must never
    // commit the desired-state revision or leave an orphan audit record.
    assert(f.database.execute(
        "CREATE TABLE addon_audit_probe (entry TEXT NOT NULL);"));
    auto refusedAudit = store.updateAudited(
        actor, {"rectools", "backend-audit", false, 0}, {}, {},
        [&] {
            return f.database.execute("INSERT INTO addon_audit_probe VALUES ('rolled-back');")
                && false;
        });
    assert(refusedAudit.status == IntentStatus::unavailable);
    auto auditRead = store.read(actor, "rectools", "backend-audit");
    assert(auditRead.status == IntentStatus::ok && auditRead.intent.revision == 0);
    ++checks;

    auto audited = store.updateAudited(
        actor, {"rectools", "backend-audit", false, 0}, {}, {},
        [&] {
            return f.database.execute("INSERT INTO addon_audit_probe VALUES ('committed');");
        });
    assert(audited.status == IntentStatus::ok && audited.intent.revision == 1 &&
           !audited.intent.desiredEnabled);
    auditRead = store.read(actor, "rectools", "backend-audit");
    assert(auditRead.status == IntentStatus::ok && auditRead.intent.revision == 1);
    sqlite3_stmt* auditCount = nullptr;
    assert(sqlite3_prepare_v2(f.database.handle(),
           "SELECT COUNT(*) FROM addon_audit_probe;", -1, &auditCount, nullptr) == SQLITE_OK);
    assert(sqlite3_step(auditCount) == SQLITE_ROW);
    assert(sqlite3_column_int(auditCount, 0) == 1);
    assert(sqlite3_finalize(auditCount) == SQLITE_OK);
    ++checks;

    // The auditing callback must not run on an exact state no-op. This also
    // proves retries cannot manufacture new accounting events.
    int noOpAuditCalls = 0;
    auto auditReplay = store.updateAudited(
        actor, {"rectools", "backend-audit", false, 1}, {}, {},
        [&] { ++noOpAuditCalls; return false; });
    assert(auditReplay.status == IntentStatus::ok &&
           auditReplay.intent.revision == 1 && noOpAuditCalls == 0);
    ++checks;
    auto rejectedBlankAudit = store.updateAudited(
        actor, {"rectools", "backend-audit", false, 1}, {}, {}, {});
    assert(rejectedBlankAudit.status == IntentStatus::unavailable);
    auditRead = store.read(actor, "rectools", "backend-audit");
    assert(auditRead.status == IntentStatus::ok && auditRead.intent.revision == 1);
    ++checks;

    std::cout << "ADDON_ACTIVATION_INTENT=PASS checks=" << checks << "\n";
}
