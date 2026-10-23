#include "AddonAdministrationService.h"
#include "AccountabilityEventRepository.h"

#include <cassert>
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
    Database db;
    Fixture()
    {
        const char* temp = std::getenv("TMPDIR");
        if (!temp || !*temp) throw std::runtime_error("isolated TMPDIR required");
        std::string pattern = (fs::path(temp) / "suite-addon-admin-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        const char* created = mkdtemp(buffer.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        folder = created;
        if (!db.open((folder / "state.sqlite").string()))
            throw std::runtime_error("isolated sqlite open failed");
    }
    ~Fixture()
    {
        db.close();
        std::error_code error;
        fs::remove_all(folder, error);
    }
};

RequestSecurityContext adminActor()
{
    RequestSecurityContext actor;
    actor.actor.actorId = "admin-a";
    actor.actor.type = ActorType::User;
    actor.authenticationState = AuthenticationState::Authenticated;
    actor.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    actor.requestId = "request-1";
    actor.correlationId = "correlation-1";
    actor.grants.push_back({"role.admin", "*"});
    return actor;
}

int main()
{
    Fixture f;
    AddonActivationIntentRepository intents(f.db);
    AccountabilityEventRepository audit(f.db);
    AddonAdministrationService service(intents, audit);
    assert(intents.ensureSchema());
    assert(audit.ensureSchema());
    assert(f.db.execute(
        "CREATE TABLE addon_media_import_maintenance ("
        "module_id TEXT PRIMARY KEY CHECK(module_id='rectools'),"
        "frozen INTEGER NOT NULL CHECK(frozen IN(0,1)),"
        "revision INTEGER NOT NULL CHECK(revision>0),"
        "updated_by_actor_id TEXT NOT NULL);"));

    auto actor = adminActor();
    const AdministrationAuditContext context{"event-1", "2026-10-20T10:00:00Z"};
    auto first = service.setDesiredState(
        actor, {"rectools", "backend-a", false, 0}, {}, {}, context);
    assert(first.status == IntentStatus::ok && first.intent.revision == 1);
    auto events = audit.listAll();
    assert(events.size() == 1);
    assert(events.front().actorId == "admin-a");
    assert(events.front().requestId == "request-1");
    assert(events.front().backendId == "backend-a");
    assert(events.front().action == "addons.intent.disable");
    assert(events.front().outcome == "committed");

    // A different SQLite database must never be mistaken for an atomic
    // accountability owner, even when the supplied event appears valid.
    Database otherDatabase;
    assert(otherDatabase.open((f.folder / "other.sqlite").string()));
    AccountabilityEventRepository foreignAudit(otherDatabase);
    assert(foreignAudit.ensureSchema());
    AddonAdministrationService foreignService(intents, foreignAudit);
    const AdministrationAuditContext foreignEvent{"event-foreign", "2026-10-20T10:00:00Z"};
    assert(foreignService.setDesiredState(
        actor, {"rectools", "backend-a", false, 1}, {}, {},
        foreignEvent).status == IntentStatus::forbidden);
    assert(foreignAudit.listAll().empty());
    otherDatabase.close();

    // Reaffirming a current, persisted disabled state is idempotent:
    // there is no new revision and no second audit event.
    auto duplicate = service.setDesiredState(
        actor, {"rectools", "backend-a", false, 1}, {}, {}, context);
    assert(duplicate.status == IntentStatus::ok &&
           duplicate.intent.revision == 1);
    // A distinct mutation reusing an existing audit ID MUST still fail
    // atomically, without persisting a new desired-state record.
    auto duplicateDifferentMutation = service.setDesiredState(
        actor, {"image", "backend-a", false, 0}, {}, {}, context);
    assert(duplicateDifferentMutation.status == IntentStatus::unavailable);
    assert(intents.read(actor, "image", "backend-a").intent.revision == 0);
    auto read = intents.read(actor, "rectools", "backend-a");
    assert(read.status == IntentStatus::ok && read.intent.revision == 1);
    assert(audit.listAll().size() == 1);

    // A globally unprivileged or revoked actor can never append an event.
    auto denied = actor;
    denied.grants.clear();
    const AdministrationAuditContext another{"event-2", "2026-10-20T10:00:00Z"};
    assert(service.setDesiredState(
        denied, {"rectools", "backend-a", false, 1}, {}, {}, another).status ==
        IntentStatus::forbidden);
    denied = actor;
    denied.actor.active = false;
    assert(service.setDesiredState(
        denied, {"rectools", "backend-a", false, 1}, {}, {}, another).status ==
        IntentStatus::forbidden);

    // A stale CAS does not emit a second audit event.
    auto conflict = service.setDesiredState(
        actor, {"rectools", "backend-a", false, 0}, {}, {}, another);
    assert(conflict.status == IntentStatus::revisionConflict);
    assert(audit.listAll().size() == 1);

    // Enabling without an independently approved live handler is denied.
    auto enable = service.setDesiredState(
        actor, {"rectools", "backend-a", true, 1}, {}, {}, another);
    assert(enable.status == IntentStatus::ineligible);
    assert(audit.listAll().size() == 1);

    // Malformed calendar values must never enter durable audit history.
    for (const auto* invalid : {"2026-02-30T10:00:00Z",
                                "2025-02-29T10:00:00Z",
                                "2026-13-20T10:00:00Z",
                                "2026-10-20T24:00:00Z",
                                "2026-10-20T10:60:00Z",
                                "2026-10-20T10:00:60Z"}) {
        const AdministrationAuditContext bad{"event-invalid", invalid};
        assert(service.setDesiredState(
            actor, {"rectools", "backend-a", false, 1}, {}, {}, bad).status ==
            IntentStatus::forbidden);
    }
    assert(audit.listAll().size() == 1);
    assert(intents.read(actor, "rectools", "backend-a").intent.revision == 1);

    // Even fabricated all-positive runtime facts must not enable the
    // administrative facade before the trusted Core provider exists.
    RuntimeEvidence forged;
    forged.moduleId = "rectools";
    forged.packageName = "vdr-suite-addon-media-tools";
    forged.backendId = "backend-a";
    forged.agentId = "agent-a";
    forged.agentInstanceId = "instance-a";
    forged.backendGeneration = 19;
    forged.installed = true;
    forged.manifestValid = true;
    forged.packageTrusted = true;
    forged.versionCompatible = true;
    forged.handlerRegistered = true;
    forged.handlerHealthy = true;
    BackendAccessDecision writable;
    writable.backendId = "backend-a";
    writable.backendFound = true;
    writable.allowed = true;
    writable.readOnly = false;
    assert(service.setDesiredState(
        actor, {"rectools", "backend-a", true, 1}, writable, forged,
        another).status == IntentStatus::ineligible);
    assert(audit.listAll().size() == 1);
    assert(intents.read(actor, "rectools", "backend-a").intent.revision == 1);

    // The adapter must provide BOTH the backend decision and provider facts.
    // Caller-forged optimistic values are ignored on enable.
    int trustedReads = 0;
    AddonAdministrationService trusted(intents, audit,
        [&](const std::string& module, const std::string& backend)
            -> std::optional<AddonAdministrationService::TrustedActivationSnapshot> {
            ++trustedReads;
            if (module != "rectools" || backend != "backend-a")
                return std::nullopt;
            return AddonAdministrationService::TrustedActivationSnapshot{
                writable, forged};
        });
    const auto enabled = trusted.setDesiredState(
        actor, {"rectools", "backend-a", true, 1}, {}, {},
        {"event-enabled", "2026-10-20T10:10:00Z"});
    assert(enabled.status == IntentStatus::ok &&
           enabled.intent.desiredEnabled && enabled.intent.revision == 2 &&
           enabled.intent.backendGeneration == 19 &&
           enabled.intent.agentId == "agent-a" &&
           enabled.intent.agentInstanceId == "instance-a");
    assert(trustedReads == 1 && audit.listAll().size() == 2);
    assert(f.db.execute(
        "INSERT INTO addon_media_import_maintenance "
        "VALUES ('rectools',1,1,'maintenance-admin');"));
    // Even a valid, independently injected provider cannot reaffirm enable
    // while the module is frozen; existing enabled state is not a bypass.
    assert(trusted.setDesiredState(
        actor, {"rectools", "backend-a", true, 2}, {}, {},
        {"event-frozen", "2026-10-20T10:10:00Z"}).status == IntentStatus::ineligible);
    assert(audit.listAll().size() == 2);
    const auto disabledAgain = trusted.setDesiredState(
        actor, {"rectools", "backend-a", false, 2}, {}, {},
        {"event-disabled", "2026-10-20T10:11:00Z"});
    assert(disabledAgain.status == IntentStatus::ok &&
           !disabledAgain.intent.desiredEnabled &&
           disabledAgain.intent.revision == 3);
    assert(trustedReads == 2 && audit.listAll().size() == 3);
    assert(f.db.execute("DELETE FROM addon_media_import_maintenance;"));

    AddonAdministrationService noProvider(intents, audit,
        [](const std::string&, const std::string&)
            -> std::optional<AddonAdministrationService::TrustedActivationSnapshot> {
            return std::nullopt;
        });
    assert(noProvider.setDesiredState(
        actor, {"rectools", "backend-a", true, 3}, writable, forged,
        {"event-denied", "2026-10-20T10:12:00Z"}).status ==
        IntentStatus::ineligible);
    assert(audit.listAll().size() == 3);

    // No caller-provided request/correlation identity bypass.
    actor.requestId = "";
    assert(service.setDesiredState(
        actor, {"rectools", "backend-a", false, 1}, {}, {}, another).status ==
        IntentStatus::forbidden);
    assert(audit.listAll().size() == 3);
    std::cout << "ADDON_ADMINISTRATION_AUDIT=PASS\n";
}
