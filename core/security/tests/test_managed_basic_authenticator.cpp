#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "ManagedBasicAuthenticator.h"
#include "PersistentIdentityResolver.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <cassert>
#include <map>
#include <string>

namespace
{
const std::string kPasswordHash =
    "$6$testsalt$qzmynZ3SU0S5D.QBAsFplf6HVa.jpeEdx88KlHvhGfddFSPHoEWMArwiVQ1PLzZDrJJ9Vs/zKBgHPMSwmFddx.";
const std::string kValidAuthorization =
    "Basic cGhhc2U2Mi1hZG1pbjp0ZXN0LXBhc3N3b3Jk";
const std::string kWrongAuthorization =
    "Basic cGhhc2U2Mi1hZG1pbjp3cm9uZy1wYXNzd29yZA==";

ManagedBasicConfiguration configuration()
{
    ManagedBasicConfiguration value;
    value.username = "phase62-admin";
    value.actorId = "user-phase62-admin";
    value.actorDisplayName = "Phase 62 administrator";
    value.deviceId = "device-phase62-admin";
    value.sessionId = "session-phase62-admin";
    value.credentialId = "credential-phase62-admin";
    value.grants = {
        PermissionGrant{"remote.control", "default"}
    };
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());

    SecurityIdentityProvisioningRepository provisioningRepository(database);
    const ManagedBasicConfiguration managed = configuration();
    assert(provisioningRepository.ensureIdentity(
        managed.actorId,
        ActorType::User,
        managed.actorDisplayName,
        managed.deviceId,
        "Managed Basic client",
        managed.sessionId,
        managed.credentialId,
        "managed-basic"));

    const auto actor = identityRepository.findActor(managed.actorId);
    const auto device = identityRepository.findDevice(managed.deviceId);
    const auto session = identityRepository.findSession(managed.sessionId);
    const auto credential =
        identityRepository.findCredential(managed.credentialId);
    assert(actor.has_value());
    assert(actor->displayName == managed.actorDisplayName);
    assert(device.has_value());
    assert(device->actorId == managed.actorId);
    assert(session.has_value());
    assert(session->actorId == managed.actorId);
    assert(session->deviceId == managed.deviceId);
    assert(credential.has_value());
    assert(credential->actorId == managed.actorId);
    assert(credential->credentialType == "managed-basic");

    assert(!provisioningRepository.ensureIdentity(
        managed.actorId,
        ActorType::User,
        "Conflicting display name",
        managed.deviceId,
        "Managed Basic client",
        managed.sessionId,
        managed.credentialId,
        "managed-basic"));

    CredentialVerifierRepository verifierRepository(database);
    assert(verifierRepository.ensureSchema());
    assert(verifierRepository.ensureVerifier(
        managed.credentialId,
        managed.username,
        kPasswordHash));
    assert(verifierRepository.ensureVerifier(
        managed.credentialId,
        managed.username,
        kPasswordHash));
    assert(!verifierRepository.ensureVerifier(
        managed.credentialId,
        managed.username,
        "$6$different$hash"));

    const auto verifier = verifierRepository.findByLogin(managed.username);
    assert(verifier.has_value());
    assert(verifier->credentialId == managed.credentialId);
    assert(verifier->passwordHash == kPasswordHash);

    assert(ManagedBasicAuthenticator::supportsPasswordHash(kPasswordHash));
    assert(ManagedBasicAuthenticator::supportsPasswordHash("$y$j9T$test"));
    assert(!ManagedBasicAuthenticator::supportsPasswordHash("plain-text"));
    assert(!ManagedBasicAuthenticator::supportsPasswordHash("$1$weak$hash"));

    ManagedBasicAuthenticator authenticator(
        managed,
        verifierRepository);

    std::map<std::string, std::string> headers;
    const RequestSecurityContext anonymous = authenticator.authenticate(
        headers,
        "request-anonymous",
        "");
    assert(anonymous.authenticationState == AuthenticationState::Anonymous);

    headers["Authorization"] = kValidAuthorization;
    RequestSecurityContext authenticated = authenticator.authenticate(
        headers,
        "request-valid",
        "correlation-valid");
    assert(authenticated.authenticated());
    assert(authenticated.actor.actorId == managed.actorId);
    assert(authenticated.actor.displayName == managed.actorDisplayName);
    assert(authenticated.device.has_value());
    assert(authenticated.device->deviceId == managed.deviceId);
    assert(authenticated.session.has_value());
    assert(authenticated.session->sessionId == managed.sessionId);
    assert(authenticated.credential.has_value());
    assert(authenticated.credential->credentialId == managed.credentialId);
    assert(authenticated.grants.size() == 1);
    assert(authenticated.grants.front().permission == "remote.control");
    assert(authenticated.requestId == "request-valid");
    assert(authenticated.correlationId == "correlation-valid");

    PersistentIdentityResolver resolver(identityRepository);
    authenticated = resolver.resolve(authenticated);
    assert(authenticated.authenticated());

    headers["Authorization"] = kWrongAuthorization;
    const RequestSecurityContext wrongPassword = authenticator.authenticate(
        headers,
        "request-wrong",
        "");
    assert(wrongPassword.authenticationState == AuthenticationState::Invalid);
    assert(wrongPassword.actor.actorId.empty());

    headers["Authorization"] = "Basic not-base64";
    const RequestSecurityContext malformed = authenticator.authenticate(
        headers,
        "request-malformed",
        "");
    assert(malformed.authenticationState == AuthenticationState::Invalid);

    headers["Authorization"] = "Bearer token";
    const RequestSecurityContext wrongScheme = authenticator.authenticate(
        headers,
        "request-scheme",
        "");
    assert(wrongScheme.authenticationState == AuthenticationState::Invalid);

    assert(identityRepository.revokeCredential(managed.credentialId));
    headers["Authorization"] = kValidAuthorization;
    RequestSecurityContext revoked = authenticator.authenticate(
        headers,
        "request-revoked",
        "");
    assert(revoked.authenticated());
    revoked = resolver.resolve(revoked);
    assert(!revoked.authenticated());
    assert(revoked.authenticationState == AuthenticationState::Revoked);
    assert(revoked.credential.has_value());
    assert(revoked.credential->revoked);

    return 0;
}
