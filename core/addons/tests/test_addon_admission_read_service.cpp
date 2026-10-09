#include "AddonAdmissionReadService.h"

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

namespace {
struct Fixture {
    fs::path folder;
    Database db;
    Fixture()
    {
        const char* temp = std::getenv("TMPDIR");
        if (temp == nullptr || *temp == '\0')
            throw std::runtime_error("TMPDIR must identify isolated test storage");
        std::string pattern = (fs::path(temp) / "suite-admission-XXXXXX").string();
        std::vector<char> name(pattern.begin(), pattern.end());
        name.push_back('\0');
        const char* created = mkdtemp(name.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        folder = created;
        if (!db.open((folder / "state.sqlite").string()))
            throw std::runtime_error("isolated SQLite open failed");
    }
    ~Fixture() {
        db.close();
        std::error_code error;
        fs::remove_all(folder, error);
    }
};

RequestSecurityContext account(bool administrative)
{
    RequestSecurityContext a;
    a.actor.actorId = administrative ? "admin-1" : "user-1";
    a.actor.type = ActorType::User;
    a.authenticationState = AuthenticationState::Authenticated;
    a.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    if (administrative) a.grants.push_back({"role.admin", "*"});
    else a.grants.push_back({"addons.media.import", "backend-a"});
    return a;
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

RuntimeEvidence independentEvidence()
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
}

int main()
{
    int checks = 0;
    Fixture fixture;
    AddonActivationIntentRepository intents(fixture.db);
    AddonAdmissionReadService service(intents);
    auto a = account(false);
    auto admin = account(true);
    auto b = backend();
    auto e = independentEvidence();

    // Even a forged adminEnabled=true cannot bypass missing SQLite state.
    e.administratorEnabled = true;
    auto preview = service.previewMediaImport(a, b, e);
    assert(!preview.policy.allowed &&
           preview.policy.reason == AccessReason::activationStateUnavailable &&
           !preview.executable);
    ++checks;

    assert(intents.ensureSchema());
    preview = service.previewMediaImport(a, b, e);
    assert(!preview.policy.allowed && preview.policy.reason == AccessReason::disabled &&
           preview.observedIntentRevision == 0 && !preview.executable);
    ++checks;

    // Administrator's role does not substitute for scoped media import.
    preview = service.previewMediaImport(admin, b, e);
    assert(!preview.policy.allowed &&
           preview.policy.reason == AccessReason::permissionDenied);
    ++checks;

    e.administratorEnabled = false;
    auto enable = intents.update(admin, {"rectools", "backend-a", true, 0}, b, e);
    assert(enable.status == IntentStatus::ok && enable.intent.revision == 1);
    ++checks;

    preview = service.previewMediaImport(a, b, e);
    assert(preview.policy.allowed && !preview.executable &&
           preview.observedIntentRevision == 1);
    ++checks;

    // Persisted preference alone cannot override revoked/missing rights.
    a.grants.clear();
    preview = service.previewMediaImport(a, b, e);
    assert(!preview.policy.allowed && preview.policy.reason == AccessReason::permissionDenied &&
           !preview.executable);
    ++checks;
    a.grants.push_back({"addons.media.import", "backend-b"});
    assert(!service.previewMediaImport(a, b, e).policy.allowed);
    ++checks;
    a = account(false);
    a.permissionGrantResolution = PermissionGrantResolutionState::Unavailable;
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::grantsUnavailable);
    ++checks;
    a = account(false);
    a.session = SessionIdentity{"s1", true, true, false};
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::unauthenticated);
    ++checks;
    a = account(false);

    e.packageTrusted = false;
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::untrustedPackage);
    ++checks;
    e = independentEvidence();
    e.manifestValid = false;
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::invalidInstallation);
    ++checks;
    e = independentEvidence();
    e.versionCompatible = false;
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::incompatibleVersion);
    ++checks;
    e = independentEvidence();
    e.handlerRegistered = false;
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::handlerUnavailable);
    ++checks;
    e = independentEvidence();
    e.handlerHealthy = false;
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::handlerUnavailable);
    ++checks;

    e = independentEvidence();
    b.allowed = false; b.readOnly = true;
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::backendUnavailable);
    ++checks;
    b = backend();
    e.backendId = "backend-b";
    assert(service.previewMediaImport(a, b, e).policy.reason == AccessReason::backendUnavailable);
    ++checks;

    // A desired-enabled row must not cross backend IDs.
    b.backendId = "backend-b";
    a.grants.clear();
    a.grants.push_back({"addons.media.import", "backend-b"});
    auto backB = service.previewMediaImport(a, b, e);
    assert(!backB.policy.allowed && backB.policy.reason == AccessReason::disabled &&
           backB.observedIntentRevision == 0);
    ++checks;

    // Admin can disable even with failed handler/backend after rev1.
    b = backend(); e = independentEvidence();
    auto disabled = intents.update(admin, {"rectools", "backend-a", false, 1}, {}, {});
    assert(disabled.status == IntentStatus::ok && disabled.intent.revision == 2);
    ++checks;
    a = account(false);
    preview = service.previewMediaImport(a, b, e);
    assert(!preview.policy.allowed && preview.policy.reason == AccessReason::disabled &&
           preview.observedIntentRevision == 2 && !preview.executable);
    ++checks;

    // Schema destruction simulates a broken/unavailable persistent owner.
    assert(fixture.db.execute("DROP TABLE addon_activation_intents;"));
    preview = service.previewMediaImport(a, b, e);
    assert(!preview.policy.allowed &&
           preview.policy.reason == AccessReason::activationStateUnavailable);
    ++checks;

    std::cout << "ADDON_ADMISSION_PREVIEW=PASS checks=" << checks << "\n";
}
