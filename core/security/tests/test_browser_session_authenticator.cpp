#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
const std::string kSessionSecret =
    "session-secret-0123456789abcdef0123456789";
const std::string kCsrfSecret =
    "csrf-secret-0123456789abcdef012345678901";
const std::string kWrongSecret =
    "wrong-session-0123456789abcdef0123456789";
const std::string kSessionSecretHash =
    "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
const std::string kCsrfSecretHash =
    "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";

std::map<std::string, std::string> cookieHeaders(
    const std::string& tokenId,
    const std::string& secret)
{
    return {
        {"Cookie", "theme=dark; vdr_suite_session=" +
            tokenId + "." + secret + "; language=de"}
    };
}

BrowserSessionCredentialRegistration registration(
    const std::string& tokenId,
    const std::string& sessionId,
    const std::string& credentialId,
    const std::string& expiresAt)
{
    BrowserSessionCredentialRegistration value;
    value.tokenId = tokenId;
    value.sessionId = sessionId;
    value.actorId = "user-phase62-admin";
    value.deviceId = "device-phase62-browser";
    value.credentialId = credentialId;
    value.issuedFromCredentialId = "credential-phase62-admin";
    value.sessionSecretHash = kSessionSecretHash;
    value.csrfSecretHash = kCsrfSecretHash;
    value.expiresAt = expiresAt;
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
    assert(provisioningRepository.ensureIdentity(
        "user-phase62-admin",
        ActorType::User,
        "Phase 62 administrator",
        "device-phase62-browser",
        "Phase 62 browser",
        "session-phase62-source",
        "credential-phase62-admin",
        "managed-basic"));
    assert(provisioningRepository.ensureIdentity(
        "user-phase62-admin",
        ActorType::User,
        "Phase 62 administrator",
        "device-phase62-browser",
        "Phase 62 browser",
        "session-browser-active",
        "credential-browser-active",
        "browser-session"));
    assert(provisioningRepository.ensureIdentity(
        "user-phase62-admin",
        ActorType::User,
        "Phase 62 administrator",
        "device-phase62-browser",
        "Phase 62 browser",
        "session-browser-expired",
        "credential-browser-expired",
        "browser-session"));

    BrowserSessionCredentialRepository repository(database);
    assert(repository.ensureSchema());

    const BrowserSessionCredentialRegistration active = registration(
        "sessiontoken001",
        "session-browser-active",
        "credential-browser-active",
        "2099-01-01 00:00:00");
    assert(repository.insert(active));
    assert(!repository.insert(active));

    BrowserSessionCredentialRegistration unsupported = registration(
        "unsupportedtoken",
        "session-browser-unsupported",
        "credential-browser-unsupported",
        "2099-01-01 00:00:00");
    unsupported.sessionSecretHash = "$2b$unsupported";
    assert(!repository.insert(unsupported));

    const auto stored = repository.findByTokenId("sessiontoken001");
    assert(stored.has_value());
    assert(stored->sessionId == "session-browser-active");
    assert(stored->credentialId == "credential-browser-active");
    assert(stored->issuedFromCredentialId == "credential-phase62-admin");
    assert(stored->sessionSecretHash == kSessionSecretHash);
    assert(stored->csrfSecretHash == kCsrfSecretHash);
    assert(stored->sessionSecretHash != kSessionSecret);
    assert(stored->sessionSecretHash != kCsrfSecret);
    assert(stored->csrfSecretHash != kSessionSecret);
    assert(stored->csrfSecretHash != kCsrfSecret);
    assert(stored->active);
    assert(!stored->expired);
    assert(!stored->revoked);

    SecurityPermissionGrantRepository grantRepository(database);
    assert(grantRepository.ensureSchema());
    assert(grantRepository.ensureGrant(
        "user-phase62-admin",
        "remote.control",
        "default"));

    BrowserSessionAuthenticator authenticator(
        repository,
        grantRepository);

    const RequestSecurityContext anonymous =
        authenticator.authenticate({}, "request-anonymous", "");
    assert(anonymous.authenticationState == AuthenticationState::Anonymous);

    const auto validHeaders =
        cookieHeaders("sessiontoken001", kSessionSecret);
    const RequestSecurityContext authenticated =
        authenticator.authenticate(
            validHeaders,
            "request-authenticated",
            "correlation-authenticated");
    assert(authenticated.authenticated());
    assert(authenticated.actor.actorId == "user-phase62-admin");
    assert(authenticated.device.has_value());
    assert(authenticated.device->deviceId == "device-phase62-browser");
    assert(authenticated.session.has_value());
    assert(authenticated.session->sessionId == "session-browser-active");
    assert(authenticated.credential.has_value());
    assert(authenticated.credential->credentialId ==
        "credential-browser-active");
    assert(authenticated.permissionGrantResolution ==
        PermissionGrantResolutionState::Resolved);
    assert(authenticated.grants.size() == 1);
    assert(authenticated.grants.front().permission == "remote.control");
    assert(authenticated.grants.front().backendId == "default");
    assert(authenticated.requestId == "request-authenticated");
    assert(authenticated.correlationId == "correlation-authenticated");

    auto csrfHeaders = validHeaders;
    csrfHeaders["X-CSRF-Token"] = kCsrfSecret;
    assert(authenticator.verifyCsrf(csrfHeaders));
    csrfHeaders["X-CSRF-Token"] =
        "wrong-csrf-0123456789abcdef012345678901";
    assert(!authenticator.verifyCsrf(csrfHeaders));
    csrfHeaders.erase("X-CSRF-Token");
    assert(!authenticator.verifyCsrf(csrfHeaders));

    assert(grantRepository.revokeGrant(
        "user-phase62-admin",
        "remote.control",
        "default"));
    const RequestSecurityContext authenticatedWithoutGrants =
        authenticator.authenticate(
            validHeaders,
            "request-without-grants",
            "");
    assert(authenticatedWithoutGrants.authenticated());
    assert(authenticatedWithoutGrants.permissionGrantResolution ==
        PermissionGrantResolutionState::Resolved);
    assert(authenticatedWithoutGrants.grants.empty());

    assert(grantRepository.ensureGrant(
        "user-phase62-admin",
        "remote.control",
        "default"));

    Database closedGrantDatabase;
    SecurityPermissionGrantRepository unavailableGrantRepository(
        closedGrantDatabase);
    BrowserSessionAuthenticator unavailableGrantAuthenticator(
        repository,
        unavailableGrantRepository);
    const RequestSecurityContext unavailableGrantContext =
        unavailableGrantAuthenticator.authenticate(
            validHeaders,
            "request-unavailable-grants",
            "");
    assert(unavailableGrantContext.authenticated());
    assert(unavailableGrantContext.permissionGrantResolution ==
        PermissionGrantResolutionState::Unavailable);
    assert(unavailableGrantContext.grants.empty());

    const RequestSecurityContext wrongSecret =
        authenticator.authenticate(
            cookieHeaders("sessiontoken001", kWrongSecret),
            "request-wrong",
            "");
    assert(wrongSecret.authenticationState == AuthenticationState::Invalid);
    assert(wrongSecret.actor.actorId.empty());

    const RequestSecurityContext unknownToken =
        authenticator.authenticate(
            cookieHeaders("unknownsession", kSessionSecret),
            "request-unknown",
            "");
    assert(unknownToken.authenticationState == AuthenticationState::Invalid);

    const RequestSecurityContext malformedToken =
        authenticator.authenticate(
            {{"Cookie", "vdr_suite_session=missing-separator"}},
            "request-malformed",
            "");
    assert(malformedToken.authenticationState == AuthenticationState::Invalid);

    const RequestSecurityContext duplicateCookie =
        authenticator.authenticate(
            {{"Cookie", "vdr_suite_session=sessiontoken001." +
                kSessionSecret + "; vdr_suite_session=sessiontoken001." +
                kSessionSecret}},
            "request-duplicate",
            "");
    assert(duplicateCookie.authenticationState == AuthenticationState::Invalid);

    const BrowserSessionCredentialRegistration expired = registration(
        "expiredtoken001",
        "session-browser-expired",
        "credential-browser-expired",
        "2000-01-01 00:00:00");
    assert(repository.insert(expired));
    const RequestSecurityContext expiredContext =
        authenticator.authenticate(
            cookieHeaders("expiredtoken001", kSessionSecret),
            "request-expired",
            "");
    assert(expiredContext.authenticationState == AuthenticationState::Expired);
    assert(expiredContext.session.has_value());
    assert(expiredContext.session->expired);
    assert(!authenticator.verifyCsrf(
        cookieHeaders("expiredtoken001", kSessionSecret)));

    assert(repository.revokeBySessionId("session-browser-active"));
    const auto revokedRecord =
        repository.findBySessionId("session-browser-active");
    assert(revokedRecord.has_value());
    assert(!revokedRecord->active);
    assert(revokedRecord->revoked);

    const RequestSecurityContext revokedContext =
        authenticator.authenticate(
            validHeaders,
            "request-revoked",
            "");
    assert(revokedContext.authenticationState == AuthenticationState::Revoked);
    assert(revokedContext.session.has_value());
    assert(revokedContext.session->revoked);
    assert(!authenticator.verifyCsrf(csrfHeaders));

    return 0;
}
