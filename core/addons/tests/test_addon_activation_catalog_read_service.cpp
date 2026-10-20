#include "AddonActivationCatalogReadService.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace vdrsuite::addons;

struct Fixture {
    fs::path folder;
    Database db;
    Fixture()
    {
        const char* tmp = std::getenv("TMPDIR");
        if (!tmp || !*tmp) throw std::runtime_error("isolated TMPDIR required");
        std::string pattern = (fs::path(tmp) / "suite-addon-catalog-XXXXXX").string();
        std::vector<char> b(pattern.begin(), pattern.end());
        b.push_back('\0');
        const char* made = mkdtemp(b.data());
        if (!made) throw std::runtime_error("mkdtemp failed");
        folder = made;
        if (!db.open((folder / "state.sqlite").string()))
            throw std::runtime_error("db open failed");
    }
    ~Fixture() { db.close(); std::error_code ec; fs::remove_all(folder, ec); }
};

RequestSecurityContext globalAdmin()
{
    RequestSecurityContext context;
    context.actor.actorId = "administrator";
    context.actor.type = ActorType::User;
    context.authenticationState = AuthenticationState::Authenticated;
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    context.grants.push_back({"role.admin", "*"});
    return context;
}

int main()
{
    Fixture fixture;
    AddonActivationIntentRepository intents(fixture.db);
    AddonActivationCatalogReadService service(intents);
    auto admin = globalAdmin();
    assert(service.read(admin, "backend-a").status == CatalogReadStatus::unavailable);
    assert(intents.ensureSchema());

    auto result = service.read(admin, "backend-a");
    assert(result.status == CatalogReadStatus::ok && result.modules.size() == 3);
    assert(result.modules[0].moduleId == "rectools");
    assert(result.modules[0].packageName == "vdr-suite-addon-media-tools");
    assert(result.modules[1].moduleId == "image");
    assert(result.modules[1].packageName == "vdr-suite-addon-image");
    assert(result.modules[2].moduleId == "music");
    assert(result.modules[2].packageName == "vdr-suite-addon-music");
    for (const auto& module : result.modules)
        assert(!module.desiredEnabled && !module.active && module.revision == 0);

    // Synthetic eligibility can record an admin desire in isolated SQLite;
    // it must still NEVER become an effective capability in the catalog.
    RuntimeEvidence evidence;
    evidence.moduleId = "rectools";
    evidence.packageName = "vdr-suite-addon-media-tools";
    evidence.backendId = "backend-a";
    evidence.installed = evidence.manifestValid = evidence.packageTrusted =
        evidence.versionCompatible = evidence.handlerRegistered =
        evidence.handlerHealthy = true;
    BackendAccessDecision backend;
    backend.backendId = "backend-a";
    backend.backendFound = backend.allowed = true;
    backend.readOnly = false;
    auto enabled = intents.update(
        admin, {"rectools", "backend-a", true, 0}, backend, evidence);
    assert(enabled.status == IntentStatus::ok);
    result = service.read(admin, "backend-a");
    assert(result.status == CatalogReadStatus::ok && result.modules.size() == 3);
    assert(result.modules[0].desiredEnabled && result.modules[0].revision == 1);
    for (const auto& module : result.modules)
        assert(!module.active);
    result = service.read(admin, "backend-b");
    assert(result.status == CatalogReadStatus::ok &&
           !result.modules[0].desiredEnabled && result.modules[0].revision == 0);

    auto denied = admin;
    denied.grants.clear();
    assert(service.read(denied, "backend-a").status == CatalogReadStatus::forbidden);
    denied = admin;
    denied.actor.active = false;
    assert(service.read(denied, "backend-a").status == CatalogReadStatus::forbidden);
    assert(service.read(admin, "../backend").status == CatalogReadStatus::unavailable);
    assert(service.read(admin, "*").status == CatalogReadStatus::unavailable);
    std::cout << "ADDON_ACTIVATION_CATALOG=PASS\n";
}
