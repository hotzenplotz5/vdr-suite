#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DeviceCredentialVerifierRepository.h"
#include "DeviceCredentialAuthenticator.h"
#include "DeviceGrantAdministrationService.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <crypt.h>
#include <algorithm>
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
        "media.live.play", "default"));
    assert(DeviceGrantAdministrationService::supportedGrant(
        "recordings.view", "default"));
    assert(DeviceGrantAdministrationService::supportedGrant(
        "epg.view", "default"));
    assert(DeviceGrantAdministrationService::supportedGrant(
        "searchtimers.view", "default"));
    assert(DeviceGrantAdministrationService::supportedGrant(
        "searchtimers.create", "default"));
    assert(DeviceGrantAdministrationService::supportedGrant(
        "timers.create", "default"));
    assert(!DeviceGrantAdministrationService::supportedGrant(
        "timers.create", "*"));
    assert(!DeviceGrantAdministrationService::supportedGrant(
        "timers.create", "."));
    assert(!DeviceGrantAdministrationService::supportedGrant(
        "searchtimers.create", "*"));
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

    // Native one-shot Timer CREATE can be issued by the real administrator
    // service and subsequently authenticated for exactly that backend.
    {
        Fixture nativeTimerGrants;
        const auto initial = nativeTimerGrants.service.read(DeviceId);
        assert(initial.status == DeviceGrantAdministrationStatus::ok);
        const auto deniedWildcard = nativeTimerGrants.service.setGrant(
            admin(), DeviceId, initial.grantSet.revision,
            "timers.create", "*", true);
        assert(deniedWildcard.status == DeviceGrantAdministrationStatus::invalid);
        const auto permitted = nativeTimerGrants.service.setGrant(
            admin(), DeviceId, initial.grantSet.revision,
            "timers.create", "default", true);
        assert(permitted.status == DeviceGrantAdministrationStatus::ok);
        assert(permitted.grantSet.grants.size() == 1U);
        DeviceCredentialAuthenticator nativeTimerAuth(
            nativeTimerGrants.verifiers, nativeTimerGrants.identities,
            nativeTimerGrants.grants);
        const auto actor = nativeTimerAuth.authenticate(
            authHeaders, "native-timer-device-grant", "");
        assert(actor.authenticated());
        assert(actor.grants.size() == 1U);
        assert(actor.grants[0].permission == "timers.create");
        assert(actor.grants[0].backendId == "default");
        const auto revoked = nativeTimerGrants.service.setGrant(
            admin(), DeviceId, permitted.grantSet.revision,
            "timers.create", "default", false);
        assert(revoked.status == DeviceGrantAdministrationStatus::ok);
        assert(revoked.grantSet.grants.empty());
        const auto afterRevocation = nativeTimerAuth.authenticate(
            authHeaders, "native-timer-device-grant-revoked", "");
        assert(afterRevocation.authenticated());
        assert(afterRevocation.grants.empty());
    }

    // Public-v1 search permissions are independently assignable to a Device.
    {
        Fixture searchGrants;
        const auto empty = searchGrants.service.read(DeviceId);
        assert(empty.status == DeviceGrantAdministrationStatus::ok);
        const auto recording = searchGrants.service.setGrant(
            admin(), DeviceId, empty.grantSet.revision,
            "recordings.view", "default", true);
        assert(recording.status == DeviceGrantAdministrationStatus::ok);
        const auto epg = searchGrants.service.setGrant(
            admin(), DeviceId, recording.grantSet.revision,
            "epg.view", "default", true);
        assert(epg.status == DeviceGrantAdministrationStatus::ok);
        assert(epg.grantSet.grants.size() == 2U);
        DeviceCredentialAuthenticator searchAuth(
            searchGrants.verifiers, searchGrants.identities, searchGrants.grants);
        const auto authenticated = searchAuth.authenticate(
            authHeaders, "mu10e-search-grants", "");
        assert(authenticated.authenticated());
        assert(authenticated.grants.size() == 2U);
        assert(std::any_of(authenticated.grants.begin(),
            authenticated.grants.end(), [](const PermissionGrant& grant) {
                return grant.permission == "recordings.view" &&
                    grant.backendId == "default";
            }));
        assert(std::any_of(authenticated.grants.begin(),
            authenticated.grants.end(), [](const PermissionGrant& grant) {
                return grant.permission == "epg.view" &&
                    grant.backendId == "default";
            }));
    }

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
