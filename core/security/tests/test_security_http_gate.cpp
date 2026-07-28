#include "AccountabilityEventRepository.h"
#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "ManagedBasicAuthenticator.h"
#include "PersistentIdentityResolver.h"
#include "SecurityHttpGate.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <cassert>
#include <string>

namespace
{
const std::string kLegacyCredential =
    "Basic YWRtaW46dmRyLXN1aXRl";
const std::string kManagedCredential =
    "Basic cGhhc2U2Mi1hZG1pbjp0ZXN0LXBhc3N3b3Jk";
const std::string kManagedWrongCredential =
    "Basic cGhhc2U2Mi1hZG1pbjp3cm9uZy1wYXNzd29yZA==";
const std::string kManagedPasswordHash =
    "$6$testsalt$qzmynZ3SU0S5D.QBAsFplf6HVa.jpeEdx88KlHvhGfddFSPHoEWMArwiVQ1PLzZDrJJ9Vs/zKBgHPMSwmFddx.";
const std::string kBrowserSessionSecret =
    "session-secret-0123456789abcdef0123456789";
const std::string kBrowserSessionSecretHash =
    "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
const std::string kBrowserCsrfSecretHash =
    "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";

HttpServerRequest getRequest(const std::string& authorization = "")
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = "/api/backends";
    if (!authorization.empty())
    {
        request.headers["Authorization"] = authorization;
    }
    return request;
}

HttpServerRequest browserGetRequest(
    const std::string& cookie,
    const std::string& authorization = "")
{
    HttpServerRequest request = getRequest(authorization);
    request.headers["Cookie"] = cookie;
    return request;
}

HttpServerRequest remoteRequest(
    const std::string& backendId,
    const std::string& authorization = "")
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/vdr/remote/actions";
    request.body =
        "{\"backendId\":\"" + backendId +
        "\",\"operationId\":\"op-1\",\"action\":\"ok\"}";
    if (!authorization.empty())
    {
        request.headers["Authorization"] = authorization;
    }
    request.headers["X-Request-ID"] = "request-1";
    request.headers["X-Correlation-ID"] = "correlation-1";
    return request;
}

HttpServerRequest unmigratedRequest(
    const std::string& authorization)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/vdr/timers/actions/create";
    request.headers["Authorization"] = authorization;
    return request;
}

SecurityConfiguration compatibility()
{
    SecurityConfiguration configuration;
    configuration.mode =
        SecurityMode::LegacyBasicCompatibility;
    configuration.expectedAuthorizationHeader = kLegacyCredential;
    configuration.grants = {PermissionGrant{"*", "*"}};
    return configuration;
}

SecurityConfiguration enforced(
    const std::vector<PermissionGrant>& grants)
{
    SecurityConfiguration configuration;
    configuration.mode = SecurityMode::Enforced;
    configuration.expectedAuthorizationHeader = kLegacyCredential;
    configuration.grants = grants;
    return configuration;
}

ManagedBasicConfiguration managedConfiguration()
{
    ManagedBasicConfiguration configuration;
    configuration.username = "phase62-admin";
    configuration.actorId = "user-phase62-admin";
    configuration.actorDisplayName = "Phase 62 administrator";
    configuration.deviceId = "device-phase62-admin";
    configuration.sessionId = "session-phase62-admin";
    configuration.credentialId = "credential-phase62-admin";
    configuration.grants = {
        PermissionGrant{"remote.control", "default"}
    };
    return configuration;
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

    const SecurityConfiguration compatibilityConfiguration =
        compatibility();
    assert(identityRepository.ensureCompatibilityIdentity(
        compatibilityConfiguration.actorId,
        ActorType::User,
        compatibilityConfiguration.actorDisplayName,
        compatibilityConfiguration.deviceId,
        compatibilityConfiguration.sessionId,
        compatibilityConfiguration.credentialId));

    const ManagedBasicConfiguration managed =
        managedConfiguration();
    SecurityIdentityProvisioningRepository provisioningRepository(database);
    assert(provisioningRepository.ensureIdentity(
        managed.actorId,
        ActorType::User,
        managed.actorDisplayName,
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
        kManagedPasswordHash));
    ManagedBasicAuthenticator managedAuthenticator(
        managed,
        verifierRepository);

    PersistentIdentityResolver identityResolver(identityRepository);

    assert(provisioningRepository.ensureIdentity(
        managed.actorId,
        ActorType::User,
        managed.actorDisplayName,
        managed.deviceId,
        "Managed Basic client",
        "session-browser-active",
        "credential-browser-active",
        "browser-session"));

    BrowserSessionCredentialRepository browserRepository(database);
    assert(browserRepository.ensureSchema());

    BrowserSessionCredentialRegistration browserRegistration;
    browserRegistration.tokenId = "sessiontoken001";
    browserRegistration.actorId = managed.actorId;
    browserRegistration.deviceId = managed.deviceId;
    browserRegistration.sessionId = "session-browser-active";
    browserRegistration.credentialId = "credential-browser-active";
    browserRegistration.issuedFromCredentialId = managed.credentialId;
    browserRegistration.sessionSecretHash =
        kBrowserSessionSecretHash;
    browserRegistration.csrfSecretHash =
        kBrowserCsrfSecretHash;
    browserRegistration.expiresAt = "2099-01-01 00:00:00";
    assert(browserRepository.insert(browserRegistration));

    BrowserSessionAuthenticator browserAuthenticator(
        browserRepository,
        std::vector<PermissionGrant>{});

    SecurityHttpGate compatibilityGate(
        compatibilityConfiguration,
        accountabilityRepository,
        &identityResolver,
        &managedAuthenticator,
        &browserAuthenticator);

    const SecurityGateDecision anonymousCompatibility =
        compatibilityGate.evaluate(getRequest());
    assert(!anonymousCompatibility.allowed);
    assert(anonymousCompatibility.rejection.statusCode == 401);
    assert(anonymousCompatibility.rejection.body.find(
        "authentication_required") != std::string::npos);
    assert(anonymousCompatibility.rejection.body.find(
        kLegacyCredential) == std::string::npos);

    const SecurityGateDecision authenticatedCompatibility =
        compatibilityGate.evaluate(getRequest(kLegacyCredential));
    assert(authenticatedCompatibility.allowed);
    assert(authenticatedCompatibility.context.actor.actorId ==
        "legacy-local-web");
    assert(authenticatedCompatibility.context.credential.has_value());
    assert(authenticatedCompatibility.context.credential->credentialId ==
        "legacy-basic-credential");

    const SecurityGateDecision compatibilityRemote =
        compatibilityGate.evaluate(
            remoteRequest("default", kLegacyCredential));
    assert(compatibilityRemote.allowed);
    assert(compatibilityRemote.protectedMutation);
    assert(compatibilityRemote.context.requestId == "request-1");

    const SecurityGateDecision legacyUnmigrated =
        compatibilityGate.evaluate(
            unmigratedRequest(kLegacyCredential));
    assert(legacyUnmigrated.allowed);

    const SecurityGateDecision managedGet =
        compatibilityGate.evaluate(getRequest(kManagedCredential));
    assert(managedGet.allowed);
    assert(managedGet.context.actor.actorId == managed.actorId);
    assert(managedGet.context.credential.has_value());
    assert(managedGet.context.credential->credentialId ==
        managed.credentialId);

    const std::string validBrowserCookie =
        "vdr_suite_session=sessiontoken001." +
        kBrowserSessionSecret;

    const SecurityGateDecision browserPreferred =
        compatibilityGate.evaluate(
            browserGetRequest(
                validBrowserCookie,
                kManagedWrongCredential));
    assert(browserPreferred.allowed);
    assert(browserPreferred.browserSessionPresented);
    assert(browserPreferred.browserAuthenticated);
    assert(browserPreferred.context.actor.actorId == managed.actorId);
    assert(browserPreferred.context.credential.has_value());
    assert(browserPreferred.context.credential->credentialId ==
        "credential-browser-active");
    assert(browserPreferred.context.grants.empty());

    const SecurityGateDecision invalidBrowserNoFallback =
        compatibilityGate.evaluate(
            browserGetRequest(
                "vdr_suite_session=sessiontoken001."
                "wrong-session-0123456789abcdef0123456789",
                kLegacyCredential));
    assert(!invalidBrowserNoFallback.allowed);
    assert(invalidBrowserNoFallback.browserSessionPresented);
    assert(!invalidBrowserNoFallback.browserAuthenticated);
    assert(invalidBrowserNoFallback.rejection.statusCode == 401);
    assert(invalidBrowserNoFallback.rejection.body.find(
        "invalid_credentials") != std::string::npos);
    assert(invalidBrowserNoFallback.rejection.headers.find(
        "WWW-Authenticate") ==
        invalidBrowserNoFallback.rejection.headers.end());

    HttpServerRequest browserRemoteRequest =
        remoteRequest("default", kLegacyCredential);
    browserRemoteRequest.headers["Cookie"] = validBrowserCookie;

    const SecurityGateDecision browserRemoteBlocked =
        compatibilityGate.evaluate(browserRemoteRequest);
    assert(!browserRemoteBlocked.allowed);
    assert(browserRemoteBlocked.browserSessionPresented);
    assert(browserRemoteBlocked.browserAuthenticated);
    assert(browserRemoteBlocked.rejection.statusCode == 503);
    assert(browserRemoteBlocked.rejection.body.find(
        "security_policy_not_migrated") != std::string::npos);

    const SecurityGateDecision managedRemote =
        compatibilityGate.evaluate(
            remoteRequest("default", kManagedCredential));
    assert(managedRemote.allowed);
    assert(managedRemote.context.actor.actorId == managed.actorId);

    const SecurityGateDecision managedUnmigrated =
        compatibilityGate.evaluate(
            unmigratedRequest(kManagedCredential));
    assert(!managedUnmigrated.allowed);
    assert(managedUnmigrated.rejection.statusCode == 503);
    assert(managedUnmigrated.rejection.body.find(
        "security_policy_not_migrated") != std::string::npos);

    const SecurityGateDecision managedWrongPassword =
        compatibilityGate.evaluate(
            getRequest(kManagedWrongCredential));
    assert(!managedWrongPassword.allowed);
    assert(managedWrongPassword.rejection.statusCode == 401);
    assert(managedWrongPassword.rejection.body.find(
        "invalid_credentials") != std::string::npos);
    assert(managedWrongPassword.rejection.body.find(
        "wrong-password") == std::string::npos);

    SecurityHttpGate enforcedGate(
        enforced({PermissionGrant{"remote.control", "default"}}),
        accountabilityRepository,
        &identityResolver,
        &managedAuthenticator,
        &browserAuthenticator);

    assert(enforcedGate.evaluate(getRequest()).allowed);

    const SecurityGateDecision invalidEnforcedGet =
        enforcedGate.evaluate(getRequest(kManagedWrongCredential));
    assert(!invalidEnforcedGet.allowed);
    assert(invalidEnforcedGet.rejection.statusCode == 401);
    assert(invalidEnforcedGet.rejection.body.find(
        "invalid_credentials") != std::string::npos);

    const SecurityGateDecision anonymousRemote =
        enforcedGate.evaluate(remoteRequest("default"));
    assert(!anonymousRemote.allowed);
    assert(anonymousRemote.rejection.statusCode == 401);
    assert(anonymousRemote.rejection.body.find(
        "authentication_required") != std::string::npos);

    const SecurityGateDecision permittedRemote =
        enforcedGate.evaluate(
            remoteRequest("default", kLegacyCredential));
    assert(permittedRemote.allowed);

    const SecurityGateDecision managedEnforcedRemote =
        enforcedGate.evaluate(
            remoteRequest("default", kManagedCredential));
    assert(managedEnforcedRemote.allowed);

    const SecurityGateDecision wrongScope =
        enforcedGate.evaluate(
            remoteRequest("house-b", kLegacyCredential));
    assert(!wrongScope.allowed);
    assert(wrongScope.rejection.statusCode == 403);
    assert(wrongScope.rejection.body.find(
        "backend_scope_denied") != std::string::npos);

    SecurityHttpGate noPermissionGate(
        enforced({PermissionGrant{"recordings.view", "*"}}),
        accountabilityRepository,
        &identityResolver);
    const SecurityGateDecision missingPermission =
        noPermissionGate.evaluate(
            remoteRequest("default", kLegacyCredential));
    assert(!missingPermission.allowed);
    assert(missingPermission.rejection.statusCode == 403);
    assert(missingPermission.rejection.body.find(
        "permission_denied") != std::string::npos);

    HttpServerRequest invalidCredential = remoteRequest("default");
    invalidCredential.headers["Authorization"] =
        "Basic definitely-not-valid";
    const SecurityGateDecision invalid =
        enforcedGate.evaluate(invalidCredential);
    assert(!invalid.allowed);
    assert(invalid.rejection.statusCode == 401);
    assert(invalid.rejection.body.find(
        "invalid_credentials") != std::string::npos);
    assert(invalid.rejection.body.find(
        "definitely-not-valid") == std::string::npos);

    const SecurityGateDecision missingBackend =
        enforcedGate.evaluate(remoteRequest("", kLegacyCredential));
    assert(!missingBackend.allowed);
    assert(missingBackend.rejection.statusCode == 400);
    assert(missingBackend.rejection.body.find(
        "invalid_backend_scope") != std::string::npos);

    const SecurityGateDecision failClosed =
        enforcedGate.evaluate(unmigratedRequest(kLegacyCredential));
    assert(!failClosed.allowed);
    assert(failClosed.rejection.statusCode == 503);
    assert(failClosed.rejection.body.find(
        "security_policy_not_migrated") != std::string::npos);

    HttpServerResponse response;
    enforcedGate.decorateResponse(
        permittedRemote.context,
        response);
    assert(response.headers.at("X-Request-ID") == "request-1");
    assert(response.headers.at("X-Correlation-ID") ==
        "correlation-1");

    const auto events = accountabilityRepository.listAll();
    bool sawAllowed = false;
    bool sawDenied = false;
    bool sawUnmigratedDenial = false;

    for (const AccountabilityEvent& event : events)
    {
        sawAllowed = sawAllowed ||
            event.eventType == "authorization.allowed";
        sawDenied = sawDenied ||
            event.eventType == "authorization.denied";
        sawUnmigratedDenial = sawUnmigratedDenial ||
            event.reasonCode == "security_policy_not_migrated";
        assert(event.permission.find("Basic ") == std::string::npos);
        assert(event.reasonCode.find("Basic ") == std::string::npos);
        assert(event.reasonCode.find("test-password") ==
            std::string::npos);
    }

    assert(sawAllowed);
    assert(sawDenied);
    assert(sawUnmigratedDenial);

    assert(browserRepository.revokeBySessionId(
        "session-browser-active"));

    const SecurityGateDecision revokedBrowserNoFallback =
        compatibilityGate.evaluate(
            browserGetRequest(
                validBrowserCookie,
                kLegacyCredential));
    assert(!revokedBrowserNoFallback.allowed);
    assert(revokedBrowserNoFallback.browserSessionPresented);
    assert(!revokedBrowserNoFallback.browserAuthenticated);
    assert(revokedBrowserNoFallback.rejection.statusCode == 401);
    assert(revokedBrowserNoFallback.rejection.body.find(
        "credential_revoked") != std::string::npos);
    assert(revokedBrowserNoFallback.rejection.headers.find(
        "WWW-Authenticate") ==
        revokedBrowserNoFallback.rejection.headers.end());

    assert(identityRepository.revokeCredential(managed.credentialId));
    const SecurityGateDecision revokedManagedCredential =
        compatibilityGate.evaluate(
            remoteRequest("default", kManagedCredential));
    assert(!revokedManagedCredential.allowed);
    assert(revokedManagedCredential.rejection.statusCode == 401);
    assert(revokedManagedCredential.rejection.body.find(
        "credential_revoked") != std::string::npos);

    assert(identityRepository.revokeCredential(
        "legacy-basic-credential"));
    const SecurityGateDecision revokedLegacyCredential =
        compatibilityGate.evaluate(
            remoteRequest("default", kLegacyCredential));
    assert(!revokedLegacyCredential.allowed);
    assert(revokedLegacyCredential.rejection.statusCode == 401);
    assert(revokedLegacyCredential.rejection.body.find(
        "credential_revoked") != std::string::npos);

    Database closedDatabase;
    AccountabilityEventRepository unavailableRepository(
        closedDatabase);
    SecurityHttpGate unavailableGate(
        enforced({PermissionGrant{"remote.control", "default"}}),
        unavailableRepository);
    const SecurityGateDecision auditFailure =
        unavailableGate.evaluate(
            remoteRequest("default", kLegacyCredential));
    assert(!auditFailure.allowed);
    assert(auditFailure.rejection.statusCode == 503);
    assert(auditFailure.rejection.body.find(
        "accountability_unavailable") != std::string::npos);

    return 0;
}
