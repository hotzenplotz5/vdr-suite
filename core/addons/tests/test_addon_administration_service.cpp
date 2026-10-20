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

    // Duplicate audit ID must make the whole new mutation fail: no rev2.
    auto duplicate = service.setDesiredState(
        actor, {"rectools", "backend-a", false, 1}, {}, {}, context);
    assert(duplicate.status == IntentStatus::unavailable);
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

    // No caller-provided request/correlation identity bypass.
    actor.requestId = "";
    assert(service.setDesiredState(
        actor, {"rectools", "backend-a", false, 1}, {}, {}, another).status ==
        IntentStatus::forbidden);
    assert(audit.listAll().size() == 1);
    std::cout << "ADDON_ADMINISTRATION_AUDIT=PASS\n";
}
