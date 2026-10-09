#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DeviceCredentialVerifierRepository.h"
#include "DeviceCredentialAuthenticator.h"
#include "DeviceGrantAdministrationService.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <crypt.h>
#include <cassert>
#include <string>

namespace
{
constexpr const char* ActorId = "actor_device_10001";
constexpr const char* DeviceId = "device_10001";
constexpr const char* CredentialId = "credential_device_10001";
const std::string Secret = "MU10E0123456789ABCDEFGHIJKLMNOPabcdefghi";

struct Fixture
{
    Database database;
    SecurityIdentityRepository identities{database};
    SecurityIdentityProvisioningRepository provisioning{database};
    DeviceCredentialVerifierRepository verifiers{database};
    SecurityPermissionGrantRepository grants{database};
    AccountabilityEventRepository accountability{database};
    DeviceGrantAdministrationService service{
        database, identities, verifiers, grants, accountability};

    Fixture()
    {
        assert(database.open(":memory:"));
        assert(identities.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());
        assert(accountability.ensureSchema());
        assert(provisioning.ensureTechnicalIdentity(
            ActorId, ActorType::Service, "Living room VIDAA",
            DeviceId, "Living room VIDAA", CredentialId, "device-app"));
        assert(database.execute("BEGIN IMMEDIATE;"));
        crypt_data state{};
        const char* encoded = crypt_r(Secret.c_str(),
            "$6$rounds=10000$mu10e-salt0001$", &state);
        assert(encoded != nullptr);
        const std::string hash(encoded);
        assert(verifiers.insertInActiveTransaction(
            CredentialId, DeviceId, hash));
        assert(database.execute("COMMIT;"));
    }
};

DeviceGrantAdministrationContext admin()
{
    return {"admin.actor:mu10e", "mu10e.request:device", "mu10e-correlation"};
}
}

int main()
{
    Fixture fixture;
    const auto initially = fixture.service.read(DeviceId);
    assert(initially.status == DeviceGrantAdministrationStatus::ok);
    assert(initially.grantSet.deviceId == DeviceId);
    assert(initially.grantSet.actorId == ActorId);
    assert(initially.grantSet.grants.empty());
    DeviceCredentialAuthenticator authenticator(
        fixture.verifiers, fixture.identities, fixture.grants);
    const std::map<std::string, std::string> authHeaders = {
        {"Authorization", std::string(DeviceCredentialAuthenticator::Scheme)
            + CredentialId + "." + Secret}
    };
    const auto unauthorisedForContent = authenticator.authenticate(
        authHeaders, "mu10e-auth-before", "");
    assert(unauthorisedForContent.authenticated());
    assert(unauthorisedForContent.grants.empty());
    assert(initially.grantSet.revision.rfind("grant-set:", 0U) == 0U);

    assert(DeviceGrantAdministrationService::supportedGrant(
        "channels.view", "default"));
    assert(DeviceGrantAdministrationService::supportedGrant(
        "recordings.view", "default"));
    assert(!DeviceGrantAdministrationService::supportedGrant(
        "recordings.view", "bad/scope"));
    assert(DeviceGrantAdministrationService::supportedGrant(
        "media.live.play", "default"));
    assert(!DeviceGrantAdministrationService::supportedGrant(
        "role.admin", "*"));
    assert(!DeviceGrantAdministrationService::supportedGrant(
        "accounts.grants.modify", "*"));
    assert(!DeviceGrantAdministrationService::supportedGrant(
        "remote.control", "*"));
    assert(!DeviceGrantAdministrationService::supportedGrant(
        "channels.view", "bad/scope"));

    const auto forbidden = fixture.service.setGrant(
        admin(), DeviceId, initially.grantSet.revision,
        "role.admin", "*", true);
    assert(forbidden.status == DeviceGrantAdministrationStatus::invalid);
    assert(fixture.service.read(DeviceId).grantSet.grants.empty());

    const auto granted = fixture.service.setGrant(
        admin(), DeviceId, initially.grantSet.revision,
        "channels.view", "default", true);
    assert(granted.status == DeviceGrantAdministrationStatus::ok);
    assert(granted.grantSet.grants.size() == 1U);
    assert(granted.grantSet.grants[0].permission == "channels.view");
    assert(granted.grantSet.grants[0].backendId == "default");
    assert(granted.grantSet.revision != initially.grantSet.revision);
    const auto withChannelGrant = authenticator.authenticate(
        authHeaders, "mu10e-auth-after", "");
    assert(withChannelGrant.authenticated());
    assert(withChannelGrant.grants.size() == 1U);
    assert(withChannelGrant.grants.front().permission == "channels.view");

    const auto alreadyActive = fixture.service.setGrant(
        admin(), DeviceId, initially.grantSet.revision,
        "channels.view", "default", true);
    assert(alreadyActive.status == DeviceGrantAdministrationStatus::ok);
    assert(alreadyActive.grantSet.revision == granted.grantSet.revision);

    const auto stale = fixture.service.setGrant(
        admin(), DeviceId, initially.grantSet.revision,
        "media.live.play", "default", true);
    assert(stale.status == DeviceGrantAdministrationStatus::revisionConflict);
    assert(fixture.service.read(DeviceId).grantSet.grants.size() == 1U);

    const auto playback = fixture.service.setGrant(
        admin(), DeviceId, granted.grantSet.revision,
        "media.live.play", "default", true);
    assert(playback.status == DeviceGrantAdministrationStatus::ok);
    assert(playback.grantSet.grants.size() == 2U);

    const auto revoked = fixture.service.setGrant(
        admin(), DeviceId, playback.grantSet.revision,
        "channels.view", "default", false);
    assert(revoked.status == DeviceGrantAdministrationStatus::ok);
    assert(revoked.grantSet.grants.size() == 1U);
    assert(revoked.grantSet.grants.front().permission ==
        "media.live.play");

    const auto repeatedRevoke = fixture.service.setGrant(
        admin(), DeviceId, initially.grantSet.revision,
        "channels.view", "default", false);
    assert(repeatedRevoke.status == DeviceGrantAdministrationStatus::ok);

    assert(fixture.service.read("unknown_device").status ==
        DeviceGrantAdministrationStatus::notFound);
    assert(fixture.service.read("../device").status ==
        DeviceGrantAdministrationStatus::invalid);

    assert(fixture.identities.revokeDevice(DeviceId));
    assert(fixture.service.read(DeviceId).status ==
        DeviceGrantAdministrationStatus::notFound);
    assert(!authenticator.authenticate(
        authHeaders, "mu10e-auth-revoked", "").authenticated());
    assert(fixture.service.setGrant(
        admin(), DeviceId, revoked.grantSet.revision,
        "channels.view", "default", true).status ==
        DeviceGrantAdministrationStatus::notFound);

    // Canonical device grants are persisted for a Service Actor only.
    const auto actorGrants = fixture.grants.findActiveGrantsForActor(ActorId);
    assert(actorGrants.available);
    assert(actorGrants.grants.size() == 1U);
    assert(actorGrants.grants.front().permission == "media.live.play");

    // A failed accountability insert must roll back a successful grant
    // SQL statement in the same write transaction.
    {
        Fixture failedAudit;
        const auto before = failedAudit.service.read(DeviceId);
        assert(before.status == DeviceGrantAdministrationStatus::ok);
        assert(failedAudit.database.execute(
            "CREATE TRIGGER mu10e_deny_audit "
            "BEFORE INSERT ON accountability_events "
            "WHEN NEW.request_id = 'mu10e-audit-failure' "
            "BEGIN SELECT RAISE(ABORT, 'audit unavailable'); END;"));
        auto rejected = failedAudit.service.setGrant(
            {"admin_actor", "mu10e-audit-failure", ""},
            DeviceId, before.grantSet.revision,
            "channels.view", "default", true);
        assert(rejected.status == DeviceGrantAdministrationStatus::unavailable);
        const auto after = failedAudit.service.read(DeviceId);
        assert(after.status == DeviceGrantAdministrationStatus::ok);
        assert(after.grantSet.grants.empty());
        assert(after.grantSet.revision == before.grantSet.revision);
    }
}
