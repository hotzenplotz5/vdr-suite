#include "AccountabilityEventRepository.h"
#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionHttpGate.h"
#include "Database.h"
#include "PersistentIdentityResolver.h"
#include "SecurityConfiguration.h"
#include "SecurityHttpGate.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <map>
#include <string>

namespace
{
const std::string kSessionSecret =
    "session-secret-0123456789abcdef0123456789";
const std::string kCsrfSecret =
    "csrf-secret-0123456789abcdef012345678901";
const std::string kSessionSecretHash =
    "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
const std::string kCsrfSecretHash =
    "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";

const std::string kActorId = "user-phase62-issuer-binding";
const std::string kDeviceId = "device-phase62-issuer-binding";

void ensureIdentity(
    SecurityIdentityProvisioningRepository& repository,
    const std::string& actorId,
    const std::string& deviceId,
    const std::string& sessionId,
    const std::string& credentialId,
    const std::string& credentialType)
{
    assert(repository.ensureIdentity(
        actorId,
        ActorType::User,
        "Phase 62 issuer binding actor",
        deviceId,
        "Phase 62 issuer binding device",
        sessionId,
        credentialId,
        credentialType));
}

BrowserSessionCredentialRegistration registration(
    const std::string& tokenId,
    const std::string& sessionId,
    const std::string& credentialId,
    const std::string& issuedFromCredentialId,
    const std::string& actorId = kActorId,
    const std::string& deviceId = kDeviceId)
{
    BrowserSessionCredentialRegistration value;
    value.tokenId = tokenId;
    value.sessionId = sessionId;
    value.actorId = actorId;
    value.deviceId = deviceId;
    value.credentialId = credentialId;
    value.issuedFromCredentialId = issuedFromCredentialId;
    value.sessionSecretHash = kSessionSecretHash;
    value.csrfSecretHash = kCsrfSecretHash;
    value.expiresAt = "2099-01-01 00:00:00";
    return value;
}

std::map<std::string, std::string> browserHeaders(
    const std::string& tokenId,
    bool includeCsrf = true)
{
    std::map<std::string, std::string> headers = {
        {"Cookie", "vdr_suite_session=" + tokenId + "." + kSessionSecret}
    };
    if (includeCsrf)
    {
        headers["X-CSRF-Token"] = kCsrfSecret;
    }
    return headers;
}

SecurityConfiguration compatibilityConfiguration()
{
    SecurityConfiguration configuration;
    configuration.mode = SecurityMode::LegacyBasicCompatibility;
    configuration.expectedAuthorizationHeader =
        "Basic YWRtaW46dmRyLXN1aXRl";
    configuration.grants = {PermissionGrant{"*", "*"}};
    return configuration;
}

HttpServerRequest ordinaryGet(
    const std::string& tokenId,
    const std::string& requestId)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = "/api/vdr/status";
    request.headers = browserHeaders(tokenId, false);
    request.headers["X-Request-ID"] = requestId;
    return request;
}

HttpServerRequest logoutRequest(
    const std::string& tokenId,
    const std::string& requestId)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/security/browser-sessions/logout";
    request.headers = browserHeaders(tokenId, true);
    request.headers["X-Request-ID"] = requestId;
    return request;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    AccountabilityEventRepository accountabilityRepository(database);
    assert(accountabilityRepository.ensureSchema());

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());

    SecurityIdentityProvisioningRepository provisioningRepository(database);
    BrowserSessionCredentialRepository browserRepository(database);
    assert(browserRepository.ensureSchema());

    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-issuer-active",
        "credential-issuer-active",
        "managed-basic");
    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-browser-active",
        "credential-browser-active",
        "browser-session");

    SecurityPermissionGrantRepository grantRepository(database);
    assert(grantRepository.ensureSchema());
    assert(grantRepository.ensureGrant(
        kActorId,
        "remote.control",
        "default"));

    assert(browserRepository.insert(registration(
        "tokenissueractive",
        "session-browser-active",
        "credential-browser-active",
        "credential-issuer-active")));

    const auto rawActive =
        browserRepository.findByTokenId("tokenissueractive");
    const auto resolvedActive =
        browserRepository.findResolvedByTokenId("tokenissueractive");
    assert(rawActive.has_value());
    assert(resolvedActive.has_value());
    assert(rawActive->active && !rawActive->expired && !rawActive->revoked);
    assert(resolvedActive->active &&
        !resolvedActive->expired &&
        !resolvedActive->revoked);

    BrowserSessionAuthenticator authenticator(
        browserRepository,
        grantRepository);
    const auto activeContext = authenticator.authenticate(
        browserHeaders("tokenissueractive", false),
        "request-issuer-active",
        "correlation-issuer-binding");
    assert(activeContext.authenticated());
    assert(activeContext.grants.size() == 1);
    assert(authenticator.verifyCsrf(
        browserHeaders("tokenissueractive", true)));

    PersistentIdentityResolver identityResolver(identityRepository);
    SecurityHttpGate securityGate(
        compatibilityConfiguration(),
        accountabilityRepository,
        &identityResolver,
        nullptr,
        &authenticator);
    const auto activeOrdinary = securityGate.evaluate(
        ordinaryGet("tokenissueractive", "request-issuer-active-get"));
    assert(activeOrdinary.allowed);
    assert(activeOrdinary.browserAuthenticated);

    BrowserSessionHttpGate lifecycleGate(
        compatibilityConfiguration(),
        accountabilityRepository,
        browserRepository,
        grantRepository,
        &identityResolver,
        nullptr);
    const auto activeLogoutGate = lifecycleGate.evaluate(
        logoutRequest("tokenissueractive", "request-issuer-active-logout"));
    assert(activeLogoutGate.allowed);

    assert(identityRepository.revokeCredential(
        "credential-issuer-active"));

    const auto rawAfterIssuerRevocation =
        browserRepository.findByTokenId("tokenissueractive");
    const auto resolvedAfterIssuerRevocation =
        browserRepository.findResolvedByTokenId("tokenissueractive");
    assert(rawAfterIssuerRevocation.has_value());
    assert(rawAfterIssuerRevocation->active);
    assert(!rawAfterIssuerRevocation->revoked);
    assert(resolvedAfterIssuerRevocation.has_value());
    assert(!resolvedAfterIssuerRevocation->active);
    assert(resolvedAfterIssuerRevocation->revoked);

    const auto revokedContext = authenticator.authenticate(
        browserHeaders("tokenissueractive", false),
        "request-issuer-revoked",
        "correlation-issuer-binding");
    assert(revokedContext.authenticationState == AuthenticationState::Revoked);
    assert(!revokedContext.authenticated());
    assert(revokedContext.grants.empty());
    assert(revokedContext.credential.has_value());
    assert(revokedContext.credential->revoked);
    assert(!authenticator.verifyCsrf(
        browserHeaders("tokenissueractive", true)));

    const auto deniedOrdinary = securityGate.evaluate(
        ordinaryGet("tokenissueractive", "request-issuer-revoked-get"));
    assert(!deniedOrdinary.allowed);
    assert(deniedOrdinary.browserSessionPresented);
    assert(!deniedOrdinary.browserAuthenticated);
    assert(deniedOrdinary.rejection.statusCode == 401);
    assert(deniedOrdinary.rejection.body.find("credential_revoked") !=
        std::string::npos);

    const auto deniedLogout = lifecycleGate.evaluate(
        logoutRequest("tokenissueractive", "request-issuer-revoked-logout"));
    assert(!deniedLogout.allowed);
    assert(deniedLogout.rejection.statusCode == 401);
    assert(deniedLogout.rejection.body.find("credential_revoked") !=
        std::string::npos);
    assert(deniedLogout.rejection.body.find("csrf_validation_failed") ==
        std::string::npos);

    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-issuer-expired",
        "credential-issuer-expired",
        "managed-basic");
    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-browser-expired-issuer",
        "credential-browser-expired-issuer",
        "browser-session");
    assert(browserRepository.insert(registration(
        "tokenissuerexpired",
        "session-browser-expired-issuer",
        "credential-browser-expired-issuer",
        "credential-issuer-expired")));
    assert(identityRepository.setCredentialExpiry(
        "credential-issuer-expired",
        "2000-01-01 00:00:00"));
    const auto expiredResolved =
        browserRepository.findResolvedByTokenId("tokenissuerexpired");
    assert(expiredResolved.has_value());
    assert(expiredResolved->active);
    assert(expiredResolved->expired);
    assert(!expiredResolved->revoked);
    const auto expiredContext = authenticator.authenticate(
        browserHeaders("tokenissuerexpired", false),
        "request-issuer-expired",
        "");
    assert(expiredContext.authenticationState == AuthenticationState::Expired);
    assert(!authenticator.verifyCsrf(
        browserHeaders("tokenissuerexpired", true)));

    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-issuer-inactive",
        "credential-issuer-inactive",
        "managed-basic");
    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-browser-inactive-issuer",
        "credential-browser-inactive-issuer",
        "browser-session");
    assert(browserRepository.insert(registration(
        "tokenissuerinactive",
        "session-browser-inactive-issuer",
        "credential-browser-inactive-issuer",
        "credential-issuer-inactive")));
    assert(database.execute(
        "UPDATE security_credentials SET active = 0 "
        "WHERE credential_id = 'credential-issuer-inactive';"));
    const auto inactiveContext = authenticator.authenticate(
        browserHeaders("tokenissuerinactive", false),
        "request-issuer-inactive",
        "");
    assert(inactiveContext.authenticationState == AuthenticationState::Revoked);
    assert(!authenticator.verifyCsrf(
        browserHeaders("tokenissuerinactive", true)));

    const std::string otherActor = "user-phase62-other-issuer";
    const std::string otherDevice = "device-phase62-other-issuer";
    ensureIdentity(
        provisioningRepository,
        otherActor,
        otherDevice,
        "session-issuer-other-actor",
        "credential-issuer-other-actor",
        "managed-basic");
    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-browser-mismatched-issuer",
        "credential-browser-mismatched-issuer",
        "browser-session");
    assert(browserRepository.insert(registration(
        "tokenissuermismatch",
        "session-browser-mismatched-issuer",
        "credential-browser-mismatched-issuer",
        "credential-issuer-other-actor")));
    const auto mismatchResolved =
        browserRepository.findResolvedByTokenId("tokenissuermismatch");
    assert(mismatchResolved.has_value());
    assert(!mismatchResolved->active);
    assert(mismatchResolved->revoked);
    const auto mismatchContext = authenticator.authenticate(
        browserHeaders("tokenissuermismatch", false),
        "request-issuer-mismatch",
        "");
    assert(mismatchContext.authenticationState == AuthenticationState::Revoked);
    assert(!authenticator.verifyCsrf(
        browserHeaders("tokenissuermismatch", true)));

    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-issuer-missing",
        "credential-issuer-missing",
        "managed-basic");
    ensureIdentity(
        provisioningRepository,
        kActorId,
        kDeviceId,
        "session-browser-missing-issuer",
        "credential-browser-missing-issuer",
        "browser-session");
    assert(browserRepository.insert(registration(
        "tokenissuermissing",
        "session-browser-missing-issuer",
        "credential-browser-missing-issuer",
        "credential-issuer-missing")));
    assert(database.execute("PRAGMA foreign_keys = OFF;"));
    assert(database.execute(
        "DELETE FROM security_credentials "
        "WHERE credential_id = 'credential-issuer-missing';"));
    assert(database.execute("PRAGMA foreign_keys = ON;"));
    const auto missingResolved =
        browserRepository.findResolvedByTokenId("tokenissuermissing");
    assert(missingResolved.has_value());
    assert(!missingResolved->active);
    assert(missingResolved->revoked);
    const auto missingContext = authenticator.authenticate(
        browserHeaders("tokenissuermissing", false),
        "request-issuer-missing",
        "");
    assert(missingContext.authenticationState == AuthenticationState::Revoked);
    assert(!authenticator.verifyCsrf(
        browserHeaders("tokenissuermissing", true)));

    bool sawRevokedGet = false;
    bool sawRevokedLogout = false;
    bool sawUnexpectedCsrfDenial = false;
    for (const AccountabilityEvent& event : accountabilityRepository.listAll())
    {
        sawRevokedGet = sawRevokedGet ||
            (event.requestId == "request-issuer-revoked-get" &&
             event.reasonCode == "credential_revoked" &&
             event.outcome == "dispatch_denied");
        sawRevokedLogout = sawRevokedLogout ||
            (event.requestId == "request-issuer-revoked-logout" &&
             event.reasonCode == "credential_revoked" &&
             event.outcome == "dispatch_denied");
        sawUnexpectedCsrfDenial = sawUnexpectedCsrfDenial ||
            ((event.requestId == "request-issuer-revoked-get" ||
              event.requestId == "request-issuer-revoked-logout") &&
             event.reasonCode == "csrf_validation_failed");
        assert(event.reasonCode.find(kSessionSecret) == std::string::npos);
        assert(event.reasonCode.find(kCsrfSecret) == std::string::npos);
    }
    assert(sawRevokedGet);
    assert(sawRevokedLogout);
    assert(!sawUnexpectedCsrfDenial);

    assert(!browserRepository.findResolvedByTokenId(
        "unknownissuertoken").has_value());

    return 0;
}
