#include "AuthorizationService.h"
#include "BackendAccessPolicy.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "MutationOperationRepository.h"
#include "SearchTimerCreateAdmissionService.h"
#include "SearchTimerCreateDeviceAdmissionService.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

using vdrsuite::operations::MutationOperationRepository;

namespace
{
class Generation : public ISearchTimerBackendGenerationAuthority
{
public:
    std::optional<std::uint64_t> live = 7U;
    mutable int reads = 0;
    std::optional<std::uint64_t> currentGeneration(
        const std::string& backendId) const override
    {
        ++reads;
        assert(backendId == "default");
        return live;
    }
};

SearchTimerCreateDeviceAdmissionRequest request()
{
    SearchTimerCreateDeviceAdmissionRequest item;
    item.context.authenticationState = AuthenticationState::Authenticated;
    item.context.actor.actorId = "device_actor_1";
    item.context.actor.type = ActorType::Service;
    item.context.device = DeviceIdentity{"device_1",true};
    item.context.credential = CredentialIdentity{"credential_1",true,false,false};
    item.context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    item.context.grants.push_back({"searchtimers.create","default"});
    item.backendId = "default";
    item.idempotencyKey = "device-create-unique-1";
    item.requestedAt = 1000;
    item.payload.name = "Terra X";
    item.payload.query = "Terra X";
    item.payload.compareTitle = true;
    return item;
}

struct Rig
{
    Database database;
    MutationOperationRepository store{database};
    SearchTimerCreateAdmissionService coreAdmission{store};
    AuthorizationService authorization;
    BackendRegistry registry;
    BackendRegistryService backends{registry};
    BackendAccessPolicy access;
    Generation generation;
    SearchTimerCreateDeviceAdmissionService deviceAdmission{
        coreAdmission,authorization,access,backends,generation};

    Rig()
    {
        assert(database.open(":memory:"));
        assert(store.ensureSchema());
        BackendNode backend;
        backend.backendId = "default";
        backend.enabled = true;
        backend.online = true;
        backend.accessMode = "read-write";
        registry.addBackend(backend);
    }

    void noOperations()
    {
        // Probe exact scope instead of relying on auto-allocated operation IDs.
        const auto r = store.findByIdempotencyScope(
            "device_actor_1","default","SearchTimer","new",
            "searchtimer.create","device-create-unique-1");
        assert(r.status ==
            vdrsuite::operations::MutationOperationRepositoryStatus::notFound);
    }
};

void assertPreparedAndReplay()
{
    Rig r;
    const auto first = r.deviceAdmission.admit(request());
    assert(first.status == SearchTimerCreateDeviceAdmissionStatus::prepared);
    assert(first.operation.actorId == "device_actor_1");
    assert(first.operation.backendId == "default");
    assert(first.operation.backendGeneration == 7U);
    assert(first.operation.actionFamily == "searchtimer.create");
    const auto replay = r.deviceAdmission.admit(request());
    assert(replay.status == SearchTimerCreateDeviceAdmissionStatus::replayed);
    assert(replay.operation.operationId == first.operation.operationId);

    r.generation.live = 8U;
    // Once reserved, a new backend generation must NOT mint a second
    // operation for the identical actor/backend/key.
    const auto afterGeneration = r.deviceAdmission.admit(request());
    assert(afterGeneration.status ==
        SearchTimerCreateDeviceAdmissionStatus::replayed);
    assert(afterGeneration.operation.operationId == first.operation.operationId);
    assert(afterGeneration.operation.backendGeneration == 7U);

    auto changed = request();
    changed.payload.query = "Andere Suche";
    assert(r.deviceAdmission.admit(changed).status ==
        SearchTimerCreateDeviceAdmissionStatus::conflict);
    assert(r.generation.reads == 4);
}

void assertIdentityDoesNotFallback()
{
    Rig r;
    auto item = request();
    item.context.actor.type = ActorType::User;
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);
    item = request(); item.context.device.reset();
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);
    item = request(); item.context.credential.reset();
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);
    item = request(); item.context.session = SessionIdentity{"browser",true,false,false};
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);
    item = request(); item.context.credential->expired = true;
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);
    item = request(); item.context.device->active = false;
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);
    item = request(); item.context.authenticationState = AuthenticationState::Invalid;
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);
    item = request();
    item.context.permissionGrantResolution = PermissionGrantResolutionState::Unavailable;
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);
    r.noOperations();
    assert(r.generation.reads == 0);
}

void assertGrantIndependenceAndScope()
{
    Rig r;
    auto item = request();
    item.context.grants = {{"searchtimers.view","default"}};
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedPermission);
    item.context.grants = {{"epg.view","default"},{"timers.create","default"}};
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedPermission);
    item.context.grants = {{"searchtimers.create","foreign"}};
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedPermission);
    item.context.grants = {{"searchtimers.create","*"}};
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedPermission);
    item.context.grants = {{"*","default"}};
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedPermission);
    item.context.grants = {{"role.admin","default"}};
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedPermission);
    item = request();
    item.context.grants.push_back({"role.read-only","default"});
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedPermission);
    r.noOperations();
    assert(r.generation.reads == 0);
}

void assertBackendAndGeneration()
{
    Rig r;
    auto item = request();
    item.backendId = "unregistered";
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::deniedPermission);
    item = request();
    r.generation.live.reset();
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::generationUnavailable);
    r.generation.live = 0U;
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::generationUnavailable);
    r.generation.live = (std::uint64_t{1} << 63);
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::generationUnavailable);
    r.generation.live = 7U;
    item.payload.query = "";
    assert(r.deviceAdmission.admit(item).status ==
        SearchTimerCreateDeviceAdmissionStatus::invalid);
    r.noOperations();

    // Write policy and online readiness must both be satisfied.
    for (const auto& mode : {"read-only","disabled","offline"})
    {
        Rig limited;
        BackendRegistry registry;
        BackendNode backend;
        backend.backendId = "default";
        backend.accessMode = std::string(mode) == "read-only"
            ? "read-only" : "read-write";
        backend.enabled = std::string(mode) != "disabled";
        backend.online = std::string(mode) != "offline";
        registry.addBackend(backend);
        BackendRegistryService backends(registry);
        SearchTimerCreateDeviceAdmissionService service(
            limited.coreAdmission,limited.authorization,limited.access,
            backends,limited.generation);
        assert(service.admit(request()).status ==
            SearchTimerCreateDeviceAdmissionStatus::deniedBackend);
        assert(limited.generation.reads == 0);
        limited.noOperations();
    }
}
}

int main()
{
    assertPreparedAndReplay();
    assertIdentityDoesNotFallback();
    assertGrantIndependenceAndScope();
    assertBackendAndGeneration();
    std::cout << "SearchTimer Device CREATE admission gate: PASS (dormant, no backend POST)\n";
}
