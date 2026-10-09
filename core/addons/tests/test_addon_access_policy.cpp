#include "AddonAccessPolicy.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::addons;

namespace {
RequestSecurityContext actor()
{
    RequestSecurityContext a;
    a.actor.actorId = "actor-1";
    a.actor.type = ActorType::User;
    a.authenticationState = AuthenticationState::Authenticated;
    a.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
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

RuntimeEvidence evidence()
{
    RuntimeEvidence e;
    e.moduleId = "rectools";
    e.packageName = "vdr-suite-addon-media-tools";
    e.backendId = "backend-a";
    e.installed = true;
    e.manifestValid = true;
    e.packageTrusted = true;
    e.versionCompatible = true;
    e.administratorEnabled = true;
    e.handlerRegistered = true;
    e.handlerHealthy = true;
    return e;
}
}

int main()
{
    int checks = 0;
    auto a = actor();
    auto b = backend();
    auto e = evidence();

    assert(!canInspectInventory(a).allowed);
    ++checks;
    a.grants.push_back({"role.admin", "backend-a"});
    assert(!canInspectInventory(a).allowed); // backend admin != global admin
    ++checks;
    a.grants.clear();
    a.grants.push_back({"role.admin", "*"});
    assert(canInspectInventory(a).allowed);
    ++checks;
    a.permissionGrantResolution = PermissionGrantResolutionState::Unavailable;
    assert(!canInspectInventory(a).allowed);
    ++checks;
    a = actor();
    a.authenticationState = AuthenticationState::Expired;
    assert(!canInspectInventory(a).allowed);
    ++checks;

    a = actor();
    assert(canUseMediaImport(a, b, e).reason == AccessReason::permissionDenied);
    ++checks;
    a.grants.push_back({"recordings.execute", "*"});
    assert(!canUseMediaImport(a, b, e).allowed);
    ++checks;
    a.grants.push_back({"addons.media.import", "backend-other"});
    assert(!canUseMediaImport(a, b, e).allowed);
    ++checks;
    a.grants.push_back({"addons.media.import", "backend-a"});
    assert(canUseMediaImport(a, b, e).allowed);
    ++checks;

    e.packageTrusted = false;
    assert(canUseMediaImport(a, b, e).reason == AccessReason::untrustedPackage);
    ++checks;
    e = evidence(); e.handlerRegistered = false;
    assert(canUseMediaImport(a, b, e).reason == AccessReason::handlerUnavailable);
    ++checks;
    e = evidence(); e.handlerHealthy = false;
    assert(canUseMediaImport(a, b, e).reason == AccessReason::handlerUnavailable);
    ++checks;
    e = evidence(); e.administratorEnabled = false;
    assert(canUseMediaImport(a, b, e).reason == AccessReason::disabled);
    ++checks;
    e = evidence(); e.versionCompatible = false;
    assert(canUseMediaImport(a, b, e).reason == AccessReason::incompatibleVersion);
    ++checks;
    e = evidence(); e.manifestValid = false;
    assert(canUseMediaImport(a, b, e).reason == AccessReason::invalidInstallation);
    ++checks;
    e = evidence(); e.packageName = "vdr-suite-addon-music";
    assert(canUseMediaImport(a, b, e).reason == AccessReason::unknownModule);
    ++checks;
    e = evidence(); e.backendId = "backend-other";
    assert(canUseMediaImport(a, b, e).reason == AccessReason::backendUnavailable);
    ++checks;
    e = evidence(); b.allowed = false; b.readOnly = true;
    assert(canUseMediaImport(a, b, e).reason == AccessReason::backendUnavailable);
    ++checks;
    b = backend(); a = actor(); a.grants.push_back({"addons.media.import", "backend-a"});
    a.actor.active = false;
    assert(!canUseMediaImport(a, b, e).allowed);
    ++checks;
    a = actor(); a.grants.push_back({"addons.media.import", "backend-a"});
    a.device = DeviceIdentity{"device-a", false};
    assert(!canUseMediaImport(a, b, e).allowed);
    ++checks;
    a = actor(); a.grants.push_back({"addons.media.import", "backend-a"});
    a.permissionGrantResolution = PermissionGrantResolutionState::Unavailable;
    assert(!canUseMediaImport(a, b, e).allowed);
    ++checks;
    std::cout << "ADDON_ACCESS_POLICY=PASS checks=" << checks << "\n";
}
