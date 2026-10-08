#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DeviceCredentialAuthenticator.h"
#include "DeviceCredentialVerifierRepository.h"
#include "PersistentIdentityResolver.h"
#include "SecurityHttpGate.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <crypt.h>
#include <string>

namespace
{
const std::string CredentialId = "credential_device_mu10d";
const std::string ActorId = "actor_device_mu10d";
const std::string DeviceId = "device_mu10d";
const std::string Secret = "MU10D_0123456789abcdefghijklmnopqrstuvwxyzABCDE";

class Fixture
{
public:
    Fixture()
        : accountability(database),
          identity(database),
          provisioning(database),
          verifiers(database),
          grants(database),
          resolver(identity),
          authenticator(verifiers, identity, grants),
          gate(SecurityConfiguration{}, accountability,
               &resolver, nullptr, nullptr, &authenticator)
    {
        assert(database.open(":memory:"));
        assert(accountability.ensureSchema());
        assert(identity.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());
        assert(provisioning.ensureTechnicalIdentity(
            ActorId, ActorType::Service, "VIDAA TV",
            DeviceId, "VIDAA TV", CredentialId, "device-app"));

        crypt_data cryptState{};
        const char* encoded = crypt_r(
            Secret.c_str(), "$6$rounds=10000$mu10dtestsalt000$",
            &cryptState);
        assert(encoded != nullptr);
        const std::string hash(encoded);
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(verifiers.insertInActiveTransaction(
            CredentialId, DeviceId, hash));
        assert(database.execute("COMMIT;"));
    }

    SecurityGateDecision evaluate(
        const std::string& target = "/api/v1",
        const std::string& authorization = "",
        const std::string& pairingToken = "") const
    {
        HttpServerRequest request;
        request.method = "GET";
        request.path = target;
        request.headers["X-Request-ID"] = "mu10d-auth-test";
        if (!authorization.empty())
            request.headers["Authorization"] = authorization;
        if (!pairingToken.empty())
            request.headers["X-VDR-Suite-Pairing-Token"] = pairingToken;
        return gate.evaluate(request);
    }

    std::string validAuthorization() const
    {
        return std::string(DeviceCredentialAuthenticator::Scheme) +
            CredentialId + "." + Secret;
    }

    Database database;
    AccountabilityEventRepository accountability;
    SecurityIdentityRepository identity;
    SecurityIdentityProvisioningRepository provisioning;
    DeviceCredentialVerifierRepository verifiers;
    SecurityPermissionGrantRepository grants;
    PersistentIdentityResolver resolver;
    DeviceCredentialAuthenticator authenticator;
    SecurityHttpGate gate;
};
}

int main()
{
    {
        Fixture fixture;
        const auto authorized = fixture.evaluate(
            "/api/v1", fixture.validAuthorization());
        assert(authorized.allowed);
        assert(authorized.deviceAuthenticated);
        assert(authorized.context.authenticated());
        assert(authorized.context.actor.actorId == ActorId);
        assert(authorized.context.actor.type == ActorType::Service);
        assert(authorized.context.device.has_value());
        assert(authorized.context.device->deviceId == DeviceId);
        assert(authorized.context.credential.has_value());
        assert(authorized.context.credential->credentialId == CredentialId);
        assert(!authorized.context.session.has_value());
        assert(authorized.context.grants.empty());
        assert(authorized.context.permissionGrantResolution ==
            PermissionGrantResolutionState::Resolved);

        // Successful identity proof gives no administrative authorization.
        const auto denied = fixture.evaluate(
            "/api/v1/device-pairings", fixture.validAuthorization());
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(denied.rejection.body.find(Secret) == std::string::npos);
        assert(denied.rejection.body.find(CredentialId) == std::string::npos);

        // The bootstrap pairing token cannot authenticate a device.
        const auto pairing = fixture.evaluate(
            "/api/v1/device-pairings", "", Secret);
        assert(!pairing.allowed);
        assert(pairing.rejection.statusCode == 401);

        const auto badSecret = fixture.evaluate(
            "/api/v1", std::string(DeviceCredentialAuthenticator::Scheme) +
            CredentialId + ".AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        assert(!badSecret.allowed);
        assert(badSecret.rejection.statusCode == 401);
        assert(badSecret.rejection.body.find(Secret) == std::string::npos);
        assert(!badSecret.context.authenticated());

        const auto noSuchCredential = fixture.evaluate(
            "/api/v1", std::string(DeviceCredentialAuthenticator::Scheme) +
            "other-credential." + Secret);
        assert(!noSuchCredential.allowed);
        assert(noSuchCredential.rejection.statusCode == 401);

        const auto malformed = fixture.evaluate(
            "/api/v1", std::string(DeviceCredentialAuthenticator::Scheme) +
            CredentialId + ".abc.");
        assert(!malformed.allowed);
        assert(malformed.rejection.statusCode == 401);

        const auto missingSecret = fixture.evaluate(
            "/api/v1", std::string(DeviceCredentialAuthenticator::Scheme) +
            CredentialId + ".");
        assert(!missingSecret.allowed);
        assert(missingSecret.rejection.statusCode == 401);

        const auto anonymous = fixture.evaluate();
        assert(anonymous.allowed);
        assert(!anonymous.context.authenticated());

        // A valid cryptographic secret must not bypass the canonical
        // credential expiry checks.
        assert(fixture.identity.setCredentialExpiry(
            CredentialId, "2000-01-01 00:00:00"));
        const auto expired = fixture.evaluate(
            "/api/v1", fixture.validAuthorization());
        assert(!expired.allowed);
        assert(expired.rejection.statusCode == 401);
        assert(expired.context.authenticationState ==
            AuthenticationState::Expired);
    }

    {
        Fixture fixture;
        assert(fixture.identity.revokeCredential(CredentialId));
        const auto revoked = fixture.evaluate(
            "/api/v1", fixture.validAuthorization());
        assert(!revoked.allowed);
        assert(revoked.rejection.statusCode == 401);
        assert(revoked.context.authenticationState ==
            AuthenticationState::Revoked);
    }

    {
        Fixture fixture;
        assert(fixture.identity.revokeDevice(DeviceId));
        const auto revoked = fixture.evaluate(
            "/api/v1", fixture.validAuthorization());
        assert(!revoked.allowed);
        assert(revoked.rejection.statusCode == 401);
    }

    {
        Fixture fixture;
        assert(fixture.identity.revokeActor(ActorId));
        const auto revoked = fixture.evaluate(
            "/api/v1", fixture.validAuthorization());
        assert(!revoked.allowed);
        assert(revoked.rejection.statusCode == 401);
    }
    return 0;
}
