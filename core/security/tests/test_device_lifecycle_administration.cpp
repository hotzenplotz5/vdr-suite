#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DeviceCredentialAuthenticator.h"
#include "DeviceCredentialVerifierRepository.h"
#include "DeviceLifecycleAdministrationService.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <crypt.h>
#include <string>

namespace
{
const std::string DeviceId = "device_mu10e2";
const std::string CredentialId = "credential_mu10e2";
const std::string ActorId = "actor_mu10e2";
const std::string Secret =
    "MU10E2ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdef";
struct Fixture
{
    Database db;
    SecurityIdentityRepository identity{db};
    SecurityIdentityProvisioningRepository provisioning{db};
    DeviceCredentialVerifierRepository verifiers{db};
    SecurityPermissionGrantRepository grants{db};
    AccountabilityEventRepository audit{db};
    DeviceLifecycleAdministrationService service{
        db, identity, verifiers, audit};
    DeviceCredentialAuthenticator authenticator{
        verifiers, identity, grants};
    Fixture()
    {
        assert(db.open(":memory:"));
        assert(identity.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());
        assert(audit.ensureSchema());
        assert(provisioning.ensureTechnicalIdentity(
            ActorId, ActorType::Service, "Hisense VIDAA",
            DeviceId, "Hisense VIDAA", CredentialId, "device-app"));
        crypt_data state{};
        const char* encoded = crypt_r(
            Secret.c_str(), "$6$rounds=10000$mu10e2testsalt$", &state);
        assert(encoded != nullptr);
        assert(db.execute("BEGIN IMMEDIATE;"));
        assert(verifiers.insertInActiveTransaction(
            CredentialId, DeviceId, std::string(encoded)));
        assert(db.execute("COMMIT;"));
    }
    bool login(const std::string& label) const
    {
        return authenticator.authenticate({
            {"Authorization", std::string(DeviceCredentialAuthenticator::Scheme) +
                CredentialId + "." + Secret}}, label, "").authenticated();
    }
};
DeviceLifecycleContext admin(const std::string& request)
{
    return {"admin.actor:mu10e2", request, ""};
}
}

int main()
{
    {
        Fixture f;
        assert(f.login("initial-login"));
        auto before = f.service.read(
            DeviceLifecycleTarget::Credential, DeviceId, CredentialId);
        assert(before.status == DeviceLifecycleStatus::ok);
        assert(before.resource.active);
        assert(!before.resource.revoked);
        assert(before.resource.actorId == ActorId);

        auto stale = f.service.revoke(
            admin("mu10e2-stale"), DeviceLifecycleTarget::Credential,
            DeviceId, CredentialId, "device-credential-lifecycle:old:active");
        assert(stale.status == DeviceLifecycleStatus::revisionConflict);
        assert(f.login("stale-did-not-revoke"));

        auto revoked = f.service.revoke(
            admin("mu10e2-revoke-credential"),
            DeviceLifecycleTarget::Credential,
            DeviceId, CredentialId, before.resource.resourceRevision);
        assert(revoked.status == DeviceLifecycleStatus::ok);
        assert(revoked.resource.revoked);
        assert(!revoked.resource.active);
        assert(revoked.resource.resourceRevision !=
            before.resource.resourceRevision);
        assert(!f.login("credential-revoked"));

        auto repeated = f.service.revoke(
            admin("mu10e2-repeat"),
            DeviceLifecycleTarget::Credential,
            DeviceId, CredentialId, before.resource.resourceRevision);
        assert(repeated.status == DeviceLifecycleStatus::ok);
        assert(repeated.resource.revoked);

        const auto device = f.service.read(
            DeviceLifecycleTarget::Device, DeviceId);
        assert(device.status == DeviceLifecycleStatus::ok);
        assert(device.resource.active);
        assert(f.service.read(
            DeviceLifecycleTarget::Credential, DeviceId,
            "wrong_credential").status == DeviceLifecycleStatus::notFound);
        assert(f.service.read(
            DeviceLifecycleTarget::Credential, "../bad",
            CredentialId).status == DeviceLifecycleStatus::invalid);
    }
    {
        Fixture f;
        assert(f.login("device-login-before"));
        auto before = f.service.read(DeviceLifecycleTarget::Device, DeviceId);
        assert(before.status == DeviceLifecycleStatus::ok);
        assert(before.resource.active);
        auto changed = f.service.revoke(
            admin("mu10e2-device-revoke"), DeviceLifecycleTarget::Device,
            DeviceId, "", before.resource.resourceRevision);
        assert(changed.status == DeviceLifecycleStatus::ok);
        assert(changed.resource.revoked);
        assert(!f.login("device-login-after"));
        auto again = f.service.revoke(
            admin("mu10e2-device-repeat"), DeviceLifecycleTarget::Device,
            DeviceId, "", before.resource.resourceRevision);
        assert(again.status == DeviceLifecycleStatus::ok);
    }
    {
        Fixture f;
        auto before = f.service.read(
            DeviceLifecycleTarget::Credential, DeviceId, CredentialId);
        assert(f.db.execute(
            "CREATE TRIGGER deny_mu10e2_audit "
            "BEFORE INSERT ON accountability_events "
            "WHEN NEW.request_id = 'mu10e2-audit-fail' "
            "BEGIN SELECT RAISE(ABORT, 'audit blocked'); END;"));
        auto failed = f.service.revoke(
            admin("mu10e2-audit-fail"),
            DeviceLifecycleTarget::Credential,
            DeviceId, CredentialId, before.resource.resourceRevision);
        assert(failed.status == DeviceLifecycleStatus::unavailable);
        assert(f.login("audit-rollback"));
        assert(f.service.read(
            DeviceLifecycleTarget::Credential, DeviceId,
            CredentialId).resource.active);
    }
    return 0;
}
