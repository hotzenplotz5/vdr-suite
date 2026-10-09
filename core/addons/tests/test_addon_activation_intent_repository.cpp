#include "AddonActivationIntentRepository.h"

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

    // Never let inactive photo/music/tvscraper scaffolds express enabled intent.
    b = backend();
    evidence = trustedEvidence();
    for (auto module : {"image", "music", "tvscraper"}) {
        assert(store.update(actor, {module, "backend-a", true, 0}, b, evidence).status ==
               IntentStatus::ineligible);
    }
    ++checks;

    read = store.read(actor, "rectools", "backend-a");
    assert(read.status == IntentStatus::ok && !read.intent.desiredEnabled &&
           read.intent.revision == 2);
    ++checks;

    std::cout << "ADDON_ACTIVATION_INTENT=PASS checks=" << checks << "\n";
}
