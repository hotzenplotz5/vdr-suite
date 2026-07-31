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
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <string>
#include <vector>

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
const std::string kBrowserCsrfSecret =
    "csrf-secret-0123456789abcdef012345678901";
const std::string kWrongBrowserCsrfSecret =
    "wrong-csrf-0123456789abcdef012345678901";
const std::string kBrowserSessionSecretHash =
    "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
const std::string kBrowserCsrfSecretHash =
    "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";

struct TimerRoute
{
    std::string path;
    std::string permission;
};

const std::vector<TimerRoute> kTimerRoutes = {
    {"/api/vdr/timers/actions/create", "timers.create"},
    {"/api/vdr/timers/actions/update", "timers.modify"},
    {"/api/vdr/timers/actions/delete", "timers.delete"}
};

const std::vector<std::string> kChannelMoveRoutes = {
    "/api/vdr/channels/move",
    "/api/vdr/channels/actions/move"
};

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

HttpServerRequest mutationRequest(
    const std::string& path,
    const std::string& backendId,
    const std::string& authorization = "")
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = path;
    request.body =
        "{\"backendId\":\"" + backendId +
        "\",\"operationId\":\"op-1\"}";
    if (!authorization.empty())
    {
        request.headers["Authorization"] = authorization;
    }
    request.headers["X-Request-ID"] = "request-1";
    request.headers["X-Correlation-ID"] = "correlation-1";
    return request;
}

HttpServerRequest remoteRequest(
    const std::string& backendId,
    const std::string& authorization = "",
    const std::string& path = "/api/vdr/remote/actions")
{
    HttpServerRequest request = mutationRequest(
        path,
        backendId,
        authorization);
    request.body =
        "{\"backendId\":\"" + backendId +
        "\",\"operationId\":\"op-1\",\"action\":\"ok\"}";
    return request;
}

HttpServerRequest timerRequest(
    const TimerRoute& route,
    const std::string& backendId,
    const std::string& authorization = "",
    const std::string& pathOverride = "")
{
    return mutationRequest(
        pathOverride.empty() ? route.path : pathOverride,
        backendId,
        authorization);
}

HttpServerRequest channelMoveRequest(
    const std::string& path,
    const std::string& backendId,
    const std::string& authorization = "")
{
    return mutationRequest(path, backendId, authorization);
}

HttpServerRequest unmigratedRequest(const std::string& authorization = "")
{
    return mutationRequest(
        "/api/vdr/recordings/actions/execute",
        "default",
        authorization);
}

SecurityConfiguration compatibility()
{
    SecurityConfiguration configuration;
    configuration.mode = SecurityMode::LegacyBasicCompatibility;
    configuration.expectedAuthorizationHeader = kLegacyCredential;
    configuration.grants = {PermissionGrant{"*", "*"}};
    return configuration;
}

SecurityConfiguration enforced(const std::vector<PermissionGrant>& grants)
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

    const SecurityConfiguration compatibilityConfiguration = compatibility();
    assert(identityRepository.ensureCompatibilityIdentity(
        compatibilityConfiguration.actorId,
        ActorType::User,
        compatibilityConfiguration.actorDisplayName,
        compatibilityConfiguration.deviceId,
        compatibilityConfiguration.sessionId,
        compatibilityConfiguration.credentialId));

    const ManagedBasicConfiguration managed = managedConfiguration();
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
    assert(provisioningRepository.ensureIdentity(
        managed.actorId,
        ActorType::User,
        managed.actorDisplayName,
        managed.deviceId,
        "Managed Basic client",
        "session-browser-active",
        "credential-browser-active",
        "browser-session"));

    CredentialVerifierRepository verifierRepository(database);
    assert(verifierRepository.ensureSchema());
    assert(verifierRepository.ensureVerifier(
        managed.credentialId,
        managed.username,
        kManagedPasswordHash));
    ManagedBasicAuthenticator managedAuthenticator(
        managed,
        verifierRepository);

    BrowserSessionCredentialRepository browserRepository(database);
    assert(browserRepository.ensureSchema());

    SecurityPermissionGrantRepository permissionGrantRepository(database);
    assert(permissionGrantRepository.ensureSchema());

    BrowserSessionCredentialRegistration browserRegistration;
    browserRegistration.tokenId = "sessiontoken001";
    browserRegistration.actorId = managed.actorId;
    browserRegistration.deviceId = managed.deviceId;
    browserRegistration.sessionId = "session-browser-active";
    browserRegistration.credentialId = "credential-browser-active";
    browserRegistration.issuedFromCredentialId = managed.credentialId;
    browserRegistration.sessionSecretHash = kBrowserSessionSecretHash;
    browserRegistration.csrfSecretHash = kBrowserCsrfSecretHash;
    browserRegistration.expiresAt = "2099-01-01 00:00:00";
    assert(browserRepository.insert(browserRegistration));

    BrowserSessionAuthenticator browserAuthenticator(
        browserRepository,
        permissionGrantRepository);
    PersistentIdentityResolver identityResolver(identityRepository);
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
    assert(anonymousCompatibility.rejection.headers.find(
        "WWW-Authenticate") ==
        anonymousCompatibility.rejection.headers.end());

    const SecurityGateDecision authenticatedCompatibility =
        compatibilityGate.evaluate(getRequest(kLegacyCredential));
    assert(authenticatedCompatibility.allowed);
    assert(authenticatedCompatibility.context.actor.actorId ==
        "legacy-local-web");

    const SecurityGateDecision compatibilityRemote =
        compatibilityGate.evaluate(
            remoteRequest("default", kLegacyCredential));
    assert(compatibilityRemote.allowed);
    assert(compatibilityRemote.protectedMutation);
    assert(compatibilityRemote.context.requestId == "request-1");

    for (const TimerRoute& route : kTimerRoutes)
    {
        const SecurityGateDecision compatibilityTimer =
            compatibilityGate.evaluate(
                timerRequest(route, "default", kLegacyCredential));
        assert(compatibilityTimer.allowed);
        assert(compatibilityTimer.protectedMutation);
    }

    for (const std::string& path : kChannelMoveRoutes)
    {
        const SecurityGateDecision compatibilityChannelMove =
            compatibilityGate.evaluate(
                channelMoveRequest(
                    path,
                    "default",
                    kLegacyCredential));
        assert(compatibilityChannelMove.allowed);
        assert(compatibilityChannelMove.protectedMutation);
    }

    assert(compatibilityGate.evaluate(
        unmigratedRequest(kLegacyCredential)).allowed);

    const SecurityGateDecision managedGet =
        compatibilityGate.evaluate(getRequest(kManagedCredential));
    assert(managedGet.allowed);
    assert(managedGet.context.actor.actorId == managed.actorId);

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
    assert(browserPreferred.context.grants.empty());

    assert(permissionGrantRepository.ensureGrant(
        managed.actorId,
        "recordings.view",
        "default"));
    const SecurityGateDecision browserWithPersistedGrant =
        compatibilityGate.evaluate(
            browserGetRequest(validBrowserCookie));
    assert(browserWithPersistedGrant.allowed);
    assert(browserWithPersistedGrant.context.grants.size() == 1);
    assert(browserWithPersistedGrant.context.grants.front().permission ==
        "recordings.view");

    Database closedGrantDatabase;
    SecurityPermissionGrantRepository unavailableGrantRepository(
        closedGrantDatabase);
    BrowserSessionAuthenticator unavailableBrowserAuthenticator(
        browserRepository,
        unavailableGrantRepository);
    SecurityHttpGate unavailableGrantGate(
        compatibilityConfiguration,
        accountabilityRepository,
        &identityResolver,
        &managedAuthenticator,
        &unavailableBrowserAuthenticator);
    const SecurityGateDecision unavailableBrowserGrants =
        unavailableGrantGate.evaluate(
            browserGetRequest(validBrowserCookie));
    assert(!unavailableBrowserGrants.allowed);
    assert(unavailableBrowserGrants.rejection.statusCode == 503);
    assert(unavailableBrowserGrants.rejection.body.find(
        "permission_grants_unavailable") != std::string::npos);

    HttpServerRequest unavailableBrowserRemote = remoteRequest("default");
    unavailableBrowserRemote.headers["Cookie"] = validBrowserCookie;
    unavailableBrowserRemote.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    const SecurityGateDecision unavailableBrowserRemoteDecision =
        unavailableGrantGate.evaluate(unavailableBrowserRemote);
    assert(!unavailableBrowserRemoteDecision.allowed);
    assert(unavailableBrowserRemoteDecision.rejection.statusCode == 503);
    assert(unavailableBrowserRemoteDecision.rejection.body.find(
        "permission_grants_unavailable") != std::string::npos);

    const SecurityGateDecision invalidBrowserNoFallback =
        compatibilityGate.evaluate(
            browserGetRequest(
                "vdr_suite_session=sessiontoken001."
                "wrong-session-0123456789abcdef0123456789",
                kLegacyCredential));
    assert(!invalidBrowserNoFallback.allowed);
    assert(invalidBrowserNoFallback.rejection.statusCode == 401);
    assert(invalidBrowserNoFallback.rejection.headers.find(
        "WWW-Authenticate") ==
        invalidBrowserNoFallback.rejection.headers.end());

    HttpServerRequest browserRemoteMissingCsrf =
        remoteRequest("default", kLegacyCredential);
    browserRemoteMissingCsrf.headers["Cookie"] = validBrowserCookie;
    const SecurityGateDecision missingCsrf =
        compatibilityGate.evaluate(browserRemoteMissingCsrf);
    assert(!missingCsrf.allowed);
    assert(missingCsrf.protectedMutation);
    assert(missingCsrf.rejection.statusCode == 403);
    assert(missingCsrf.rejection.body.find(
        "csrf_validation_failed") != std::string::npos);
    assert(missingCsrf.rejection.headers.find("WWW-Authenticate") ==
        missingCsrf.rejection.headers.end());

    HttpServerRequest browserRemoteWrongCsrf =
        remoteRequest("default", kLegacyCredential);
    browserRemoteWrongCsrf.headers["Cookie"] = validBrowserCookie;
    browserRemoteWrongCsrf.headers["X-CSRF-Token"] =
        kWrongBrowserCsrfSecret;
    const SecurityGateDecision wrongCsrf =
        compatibilityGate.evaluate(browserRemoteWrongCsrf);
    assert(!wrongCsrf.allowed);
    assert(wrongCsrf.protectedMutation);
    assert(wrongCsrf.rejection.statusCode == 403);
    assert(wrongCsrf.rejection.body.find(
        "csrf_validation_failed") != std::string::npos);
    assert(wrongCsrf.rejection.headers.find("WWW-Authenticate") ==
        wrongCsrf.rejection.headers.end());

    HttpServerRequest browserRemoteNoPermission = remoteRequest("default");
    browserRemoteNoPermission.headers["Cookie"] = validBrowserCookie;
    browserRemoteNoPermission.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    const SecurityGateDecision noBrowserRemotePermission =
        compatibilityGate.evaluate(browserRemoteNoPermission);
    assert(!noBrowserRemotePermission.allowed);
    assert(noBrowserRemotePermission.rejection.statusCode == 403);
    assert(noBrowserRemotePermission.rejection.body.find(
        "permission_denied") != std::string::npos);

    assert(permissionGrantRepository.ensureGrant(
        managed.actorId,
        "remote.control",
        "default"));
    const SecurityGateDecision browserRemoteAllowed =
        compatibilityGate.evaluate(browserRemoteNoPermission);
    assert(browserRemoteAllowed.allowed);
    assert(browserRemoteAllowed.protectedMutation);
    assert(browserRemoteAllowed.browserAuthenticated);

    HttpServerRequest browserRemoteWrongScope = remoteRequest("house-b");
    browserRemoteWrongScope.headers["Cookie"] = validBrowserCookie;
    browserRemoteWrongScope.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    const SecurityGateDecision browserWrongScope =
        compatibilityGate.evaluate(browserRemoteWrongScope);
    assert(!browserWrongScope.allowed);
    assert(browserWrongScope.rejection.statusCode == 403);
    assert(browserWrongScope.rejection.body.find(
        "backend_scope_denied") != std::string::npos);

    HttpServerRequest browserRemoteMissingBackend = remoteRequest("");
    browserRemoteMissingBackend.headers["Cookie"] = validBrowserCookie;
    browserRemoteMissingBackend.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    const SecurityGateDecision browserMissingBackend =
        compatibilityGate.evaluate(browserRemoteMissingBackend);
    assert(!browserMissingBackend.allowed);
    assert(browserMissingBackend.rejection.statusCode == 400);
    assert(browserMissingBackend.rejection.body.find(
        "invalid_backend_scope") != std::string::npos);

    HttpServerRequest browserRemoteQuery = remoteRequest(
        "default",
        "",
        "/api/vdr/remote/actions?source=browser");
    browserRemoteQuery.headers["Cookie"] = validBrowserCookie;
    browserRemoteQuery.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    assert(compatibilityGate.evaluate(browserRemoteQuery).allowed);

    HttpServerRequest browserRemoteTrailingSlash = remoteRequest(
        "default",
        "",
        "/api/vdr/remote/actions/");
    browserRemoteTrailingSlash.headers["Cookie"] = validBrowserCookie;
    browserRemoteTrailingSlash.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    const SecurityGateDecision trailingSlashDecision =
        compatibilityGate.evaluate(browserRemoteTrailingSlash);
    assert(!trailingSlashDecision.allowed);
    assert(trailingSlashDecision.rejection.statusCode == 503);
    assert(trailingSlashDecision.rejection.body.find(
        "security_policy_not_migrated") != std::string::npos);

    for (const TimerRoute& route : kTimerRoutes)
    {
        HttpServerRequest missingTimerCsrf =
            timerRequest(route, "default");
        missingTimerCsrf.headers["Cookie"] = validBrowserCookie;
        const SecurityGateDecision missingTimerCsrfDecision =
            compatibilityGate.evaluate(missingTimerCsrf);
        assert(!missingTimerCsrfDecision.allowed);
        assert(missingTimerCsrfDecision.protectedMutation);
        assert(missingTimerCsrfDecision.rejection.statusCode == 403);
        assert(missingTimerCsrfDecision.rejection.body.find(
            "csrf_validation_failed") != std::string::npos);

        HttpServerRequest timerNoPermission =
            timerRequest(route, "default");
        timerNoPermission.headers["Cookie"] = validBrowserCookie;
        timerNoPermission.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
        const SecurityGateDecision timerNoPermissionDecision =
            compatibilityGate.evaluate(timerNoPermission);
        assert(!timerNoPermissionDecision.allowed);
        assert(timerNoPermissionDecision.rejection.statusCode == 403);
        assert(timerNoPermissionDecision.rejection.body.find(
            "permission_denied") != std::string::npos);

        assert(permissionGrantRepository.ensureGrant(
            managed.actorId,
            route.permission,
            "default"));
        const SecurityGateDecision directTimerAllowed =
            compatibilityGate.evaluate(timerNoPermission);
        assert(directTimerAllowed.allowed);
        assert(directTimerAllowed.protectedMutation);

        HttpServerRequest timerWrongScope =
            timerRequest(route, "house-b");
        timerWrongScope.headers["Cookie"] = validBrowserCookie;
        timerWrongScope.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
        const SecurityGateDecision timerWrongScopeDecision =
            compatibilityGate.evaluate(timerWrongScope);
        assert(!timerWrongScopeDecision.allowed);
        assert(timerWrongScopeDecision.rejection.statusCode == 403);
        assert(timerWrongScopeDecision.rejection.body.find(
            "backend_scope_denied") != std::string::npos);

        assert(permissionGrantRepository.revokeGrant(
            managed.actorId,
            route.permission,
            "default"));
    }

    HttpServerRequest browserChannelWrongCsrf =
        channelMoveRequest(
            kChannelMoveRoutes.back(),
            "default");
    browserChannelWrongCsrf.headers["Cookie"] = validBrowserCookie;
    browserChannelWrongCsrf.headers["X-CSRF-Token"] =
        kWrongBrowserCsrfSecret;
    const SecurityGateDecision browserChannelWrongCsrfDecision =
        compatibilityGate.evaluate(browserChannelWrongCsrf);
    assert(!browserChannelWrongCsrfDecision.allowed);
    assert(browserChannelWrongCsrfDecision.protectedMutation);
    assert(browserChannelWrongCsrfDecision.rejection.statusCode == 403);
    assert(browserChannelWrongCsrfDecision.rejection.body.find(
        "csrf_validation_failed") != std::string::npos);

    for (const std::string& path : kChannelMoveRoutes)
    {
        HttpServerRequest missingChannelCsrf =
            channelMoveRequest(path, "default");
        missingChannelCsrf.headers["Cookie"] = validBrowserCookie;
        const SecurityGateDecision missingChannelCsrfDecision =
            compatibilityGate.evaluate(missingChannelCsrf);
        assert(!missingChannelCsrfDecision.allowed);
        assert(missingChannelCsrfDecision.protectedMutation);
        assert(missingChannelCsrfDecision.rejection.statusCode == 403);
        assert(missingChannelCsrfDecision.rejection.body.find(
            "csrf_validation_failed") != std::string::npos);

        HttpServerRequest channelNoPermission =
            channelMoveRequest(path, "default");
        channelNoPermission.headers["Cookie"] = validBrowserCookie;
        channelNoPermission.headers["X-CSRF-Token"] =
            kBrowserCsrfSecret;
        const SecurityGateDecision channelNoPermissionDecision =
            compatibilityGate.evaluate(channelNoPermission);
        assert(!channelNoPermissionDecision.allowed);
        assert(channelNoPermissionDecision.rejection.statusCode == 403);
        assert(channelNoPermissionDecision.rejection.body.find(
            "permission_denied") != std::string::npos);

        assert(permissionGrantRepository.ensureGrant(
            managed.actorId,
            "channels.move",
            "default"));
        const SecurityGateDecision directChannelAllowed =
            compatibilityGate.evaluate(channelNoPermission);
        assert(directChannelAllowed.allowed);
        assert(directChannelAllowed.protectedMutation);

        HttpServerRequest channelWrongScope =
            channelMoveRequest(path, "house-b");
        channelWrongScope.headers["Cookie"] = validBrowserCookie;
        channelWrongScope.headers["X-CSRF-Token"] =
            kBrowserCsrfSecret;
        const SecurityGateDecision channelWrongScopeDecision =
            compatibilityGate.evaluate(channelWrongScope);
        assert(!channelWrongScopeDecision.allowed);
        assert(channelWrongScopeDecision.rejection.statusCode == 403);
        assert(channelWrongScopeDecision.rejection.body.find(
            "backend_scope_denied") != std::string::npos);

        assert(permissionGrantRepository.revokeGrant(
            managed.actorId,
            "channels.move",
            "default"));
    }

    HttpServerRequest timerMissingBackend =
        timerRequest(kTimerRoutes.front(), "");
    timerMissingBackend.headers["Cookie"] = validBrowserCookie;
    timerMissingBackend.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    const SecurityGateDecision timerMissingBackendDecision =
        compatibilityGate.evaluate(timerMissingBackend);
    assert(!timerMissingBackendDecision.allowed);
    assert(timerMissingBackendDecision.rejection.statusCode == 400);
    assert(timerMissingBackendDecision.rejection.body.find(
        "invalid_backend_scope") != std::string::npos);

    HttpServerRequest channelMissingBackend =
        channelMoveRequest(
            kChannelMoveRoutes.front(),
            "");
    channelMissingBackend.headers["Cookie"] = validBrowserCookie;
    channelMissingBackend.headers["X-CSRF-Token"] =
        kBrowserCsrfSecret;
    const SecurityGateDecision channelMissingBackendDecision =
        compatibilityGate.evaluate(channelMissingBackend);
    assert(!channelMissingBackendDecision.allowed);
    assert(channelMissingBackendDecision.rejection.statusCode == 400);
    assert(channelMissingBackendDecision.rejection.body.find(
        "invalid_backend_scope") != std::string::npos);

    assert(permissionGrantRepository.ensureGrant(
        managed.actorId,
        "role.admin",
        "default"));
    for (const TimerRoute& route : kTimerRoutes)
    {
        HttpServerRequest adminTimer = timerRequest(route, "default");
        adminTimer.headers["Cookie"] = validBrowserCookie;
        adminTimer.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
        const SecurityGateDecision adminTimerDecision =
            compatibilityGate.evaluate(adminTimer);
        assert(adminTimerDecision.allowed);
    }

    for (const std::string& path : kChannelMoveRoutes)
    {
        HttpServerRequest adminChannelMove =
            channelMoveRequest(path, "default");
        adminChannelMove.headers["Cookie"] = validBrowserCookie;
        adminChannelMove.headers["X-CSRF-Token"] =
            kBrowserCsrfSecret;
        const SecurityGateDecision adminChannelMoveDecision =
            compatibilityGate.evaluate(adminChannelMove);
        assert(adminChannelMoveDecision.allowed);
    }

    assert(permissionGrantRepository.ensureGrant(
        managed.actorId,
        "role.read-only",
        "default"));
    for (const TimerRoute& route : kTimerRoutes)
    {
        HttpServerRequest readOnlyTimer = timerRequest(route, "default");
        readOnlyTimer.headers["Cookie"] = validBrowserCookie;
        readOnlyTimer.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
        const SecurityGateDecision readOnlyTimerDecision =
            compatibilityGate.evaluate(readOnlyTimer);
        assert(!readOnlyTimerDecision.allowed);
        assert(readOnlyTimerDecision.rejection.statusCode == 403);
        assert(readOnlyTimerDecision.rejection.body.find(
            "role_read_only") != std::string::npos);
    }
    for (const std::string& path : kChannelMoveRoutes)
    {
        HttpServerRequest readOnlyChannelMove =
            channelMoveRequest(path, "default");
        readOnlyChannelMove.headers["Cookie"] = validBrowserCookie;
        readOnlyChannelMove.headers["X-CSRF-Token"] =
            kBrowserCsrfSecret;
        const SecurityGateDecision readOnlyChannelMoveDecision =
            compatibilityGate.evaluate(readOnlyChannelMove);
        assert(!readOnlyChannelMoveDecision.allowed);
        assert(readOnlyChannelMoveDecision.rejection.statusCode == 403);
        assert(readOnlyChannelMoveDecision.rejection.body.find(
            "role_read_only") != std::string::npos);
    }

    assert(permissionGrantRepository.revokeGrant(
        managed.actorId,
        "role.read-only",
        "default"));
    assert(permissionGrantRepository.revokeGrant(
        managed.actorId,
        "role.admin",
        "default"));

    HttpServerRequest browserTimerQuery = timerRequest(
        kTimerRoutes[1],
        "default",
        "",
        "/api/vdr/timers/actions/update?source=browser");
    browserTimerQuery.headers["Cookie"] = validBrowserCookie;
    browserTimerQuery.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    assert(permissionGrantRepository.ensureGrant(
        managed.actorId,
        "timers.modify",
        "default"));
    assert(compatibilityGate.evaluate(browserTimerQuery).allowed);
    assert(permissionGrantRepository.revokeGrant(
        managed.actorId,
        "timers.modify",
        "default"));

    HttpServerRequest browserTimerTrailingSlash = timerRequest(
        kTimerRoutes[1],
        "default",
        "",
        "/api/vdr/timers/actions/update/");
    browserTimerTrailingSlash.headers["Cookie"] = validBrowserCookie;
    browserTimerTrailingSlash.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    const SecurityGateDecision timerTrailingSlashDecision =
        compatibilityGate.evaluate(browserTimerTrailingSlash);
    assert(!timerTrailingSlashDecision.allowed);
    assert(timerTrailingSlashDecision.rejection.statusCode == 503);
    assert(timerTrailingSlashDecision.rejection.body.find(
        "security_policy_not_migrated") != std::string::npos);

    for (const std::string& path : kChannelMoveRoutes)
    {
        HttpServerRequest browserChannelQuery =
            channelMoveRequest(
                path + "?source=browser",
                "default");
        browserChannelQuery.headers["Cookie"] = validBrowserCookie;
        browserChannelQuery.headers["X-CSRF-Token"] =
            kBrowserCsrfSecret;
        assert(permissionGrantRepository.ensureGrant(
            managed.actorId,
            "channels.move",
            "default"));
        assert(compatibilityGate.evaluate(browserChannelQuery).allowed);
        assert(permissionGrantRepository.revokeGrant(
            managed.actorId,
            "channels.move",
            "default"));

        HttpServerRequest browserChannelTrailingSlash =
            channelMoveRequest(
                path + "/",
                "default");
        browserChannelTrailingSlash.headers["Cookie"] =
            validBrowserCookie;
        browserChannelTrailingSlash.headers["X-CSRF-Token"] =
            kBrowserCsrfSecret;
        const SecurityGateDecision channelTrailingSlashDecision =
            compatibilityGate.evaluate(browserChannelTrailingSlash);
        assert(!channelTrailingSlashDecision.allowed);
        assert(channelTrailingSlashDecision.rejection.statusCode == 503);
        assert(channelTrailingSlashDecision.rejection.body.find(
            "security_policy_not_migrated") != std::string::npos);
    }

    HttpServerRequest browserUnmigrated = unmigratedRequest();
    browserUnmigrated.headers["Cookie"] = validBrowserCookie;
    browserUnmigrated.headers["X-CSRF-Token"] = kBrowserCsrfSecret;
    const SecurityGateDecision browserUnmigratedDecision =
        compatibilityGate.evaluate(browserUnmigrated);
    assert(!browserUnmigratedDecision.allowed);
    assert(browserUnmigratedDecision.rejection.statusCode == 503);
    assert(browserUnmigratedDecision.rejection.body.find(
        "security_policy_not_migrated") != std::string::npos);

    const SecurityGateDecision managedRemote =
        compatibilityGate.evaluate(
            remoteRequest("default", kManagedCredential));
    assert(managedRemote.allowed);

    for (const TimerRoute& route : kTimerRoutes)
    {
        const SecurityGateDecision managedTimerDenied =
            compatibilityGate.evaluate(
                timerRequest(route, "default", kManagedCredential));
        assert(!managedTimerDenied.allowed);
        assert(managedTimerDenied.protectedMutation);
        assert(managedTimerDenied.rejection.statusCode == 403);
        assert(managedTimerDenied.rejection.body.find(
            "permission_denied") != std::string::npos);
        assert(managedTimerDenied.rejection.body.find(
            "security_policy_not_migrated") == std::string::npos);
    }

    for (const std::string& path : kChannelMoveRoutes)
    {
        const SecurityGateDecision managedChannelDenied =
            compatibilityGate.evaluate(
                channelMoveRequest(
                    path,
                    "default",
                    kManagedCredential));
        assert(!managedChannelDenied.allowed);
        assert(managedChannelDenied.protectedMutation);
        assert(managedChannelDenied.rejection.statusCode == 403);
        assert(managedChannelDenied.rejection.body.find(
            "permission_denied") != std::string::npos);
        assert(managedChannelDenied.rejection.body.find(
            "security_policy_not_migrated") == std::string::npos);
    }

    const SecurityGateDecision managedUnmigrated =
        compatibilityGate.evaluate(
            unmigratedRequest(kManagedCredential));
    assert(!managedUnmigrated.allowed);
    assert(managedUnmigrated.rejection.statusCode == 503);

    const SecurityGateDecision managedWrongPassword =
        compatibilityGate.evaluate(
            getRequest(kManagedWrongCredential));
    assert(!managedWrongPassword.allowed);
    assert(managedWrongPassword.rejection.statusCode == 401);
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

    const SecurityGateDecision anonymousRemote =
        enforcedGate.evaluate(remoteRequest("default"));
    assert(!anonymousRemote.allowed);
    assert(anonymousRemote.rejection.statusCode == 401);

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

    SecurityHttpGate timerEnforcedGate(
        enforced({
            PermissionGrant{"timers.create", "default"},
            PermissionGrant{"timers.modify", "default"},
            PermissionGrant{"timers.delete", "default"}
        }),
        accountabilityRepository,
        &identityResolver);
    for (const TimerRoute& route : kTimerRoutes)
    {
        const SecurityGateDecision enforcedTimerAllowed =
            timerEnforcedGate.evaluate(
                timerRequest(route, "default", kLegacyCredential));
        assert(enforcedTimerAllowed.allowed);

        const SecurityGateDecision enforcedTimerWrongScope =
            timerEnforcedGate.evaluate(
                timerRequest(route, "house-b", kLegacyCredential));
        assert(!enforcedTimerWrongScope.allowed);
        assert(enforcedTimerWrongScope.rejection.statusCode == 403);
        assert(enforcedTimerWrongScope.rejection.body.find(
            "backend_scope_denied") != std::string::npos);
    }

    SecurityHttpGate channelEnforcedGate(
        enforced({PermissionGrant{"channels.move", "default"}}),
        accountabilityRepository,
        &identityResolver);
    for (const std::string& path : kChannelMoveRoutes)
    {
        const SecurityGateDecision enforcedChannelAllowed =
            channelEnforcedGate.evaluate(
                channelMoveRequest(
                    path,
                    "default",
                    kLegacyCredential));
        assert(enforcedChannelAllowed.allowed);

        const SecurityGateDecision enforcedChannelWrongScope =
            channelEnforcedGate.evaluate(
                channelMoveRequest(
                    path,
                    "house-b",
                    kLegacyCredential));
        assert(!enforcedChannelWrongScope.allowed);
        assert(enforcedChannelWrongScope.rejection.statusCode == 403);
        assert(enforcedChannelWrongScope.rejection.body.find(
            "backend_scope_denied") != std::string::npos);
    }

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
    for (const TimerRoute& route : kTimerRoutes)
    {
        const SecurityGateDecision missingTimerPermission =
            noPermissionGate.evaluate(
                timerRequest(route, "default", kLegacyCredential));
        assert(!missingTimerPermission.allowed);
        assert(missingTimerPermission.rejection.statusCode == 403);
        assert(missingTimerPermission.rejection.body.find(
            "permission_denied") != std::string::npos);
    }

    for (const std::string& path : kChannelMoveRoutes)
    {
        const SecurityGateDecision missingChannelPermission =
            noPermissionGate.evaluate(
                channelMoveRequest(
                    path,
                    "default",
                    kLegacyCredential));
        assert(!missingChannelPermission.allowed);
        assert(missingChannelPermission.rejection.statusCode == 403);
        assert(missingChannelPermission.rejection.body.find(
            "permission_denied") != std::string::npos);
    }

    HttpServerRequest invalidCredential = remoteRequest("default");
    invalidCredential.headers["Authorization"] =
        "Basic definitely-not-valid";
    const SecurityGateDecision invalid =
        enforcedGate.evaluate(invalidCredential);
    assert(!invalid.allowed);
    assert(invalid.rejection.statusCode == 401);
    assert(invalid.rejection.body.find(
        "definitely-not-valid") == std::string::npos);

    const SecurityGateDecision missingBackend =
        enforcedGate.evaluate(remoteRequest("", kLegacyCredential));
    assert(!missingBackend.allowed);
    assert(missingBackend.rejection.statusCode == 400);
    assert(missingBackend.rejection.body.find(
        "invalid_backend_scope") != std::string::npos);

    const SecurityGateDecision failClosed =
        enforcedGate.evaluate(
            unmigratedRequest(kLegacyCredential));
    assert(!failClosed.allowed);
    assert(failClosed.rejection.statusCode == 503);

    HttpServerResponse response;
    enforcedGate.decorateResponse(
        permittedRemote.context,
        response);
    assert(response.headers.at("X-Request-ID") == "request-1");
    assert(response.headers.at("X-Correlation-ID") ==
        "correlation-1");

    bool sawAllowed = false;
    bool sawDenied = false;
    bool sawUnmigratedDenial = false;
    bool sawCsrfDenial = false;
    bool sawTimerCsrfDenial = false;
    bool sawTimerAllowed = false;
    bool sawChannelCsrfDenial = false;
    bool sawChannelAllowed = false;
    for (const AccountabilityEvent& event : accountabilityRepository.listAll())
    {
        sawAllowed = sawAllowed ||
            event.eventType == "authorization.allowed";
        sawDenied = sawDenied ||
            event.eventType == "authorization.denied";
        sawUnmigratedDenial = sawUnmigratedDenial ||
            event.reasonCode == "security_policy_not_migrated";
        sawCsrfDenial = sawCsrfDenial ||
            (event.permission == "remote.control" &&
             event.reasonCode == "csrf_validation_failed" &&
             event.outcome == "dispatch_denied");
        sawTimerCsrfDenial = sawTimerCsrfDenial ||
            (event.permission == "timers.create" &&
             event.reasonCode == "csrf_validation_failed" &&
             event.outcome == "dispatch_denied");
        sawTimerAllowed = sawTimerAllowed ||
            ((event.permission == "timers.create" ||
              event.permission == "timers.modify" ||
              event.permission == "timers.delete") &&
             event.outcome == "dispatch_authorized");
        sawChannelCsrfDenial = sawChannelCsrfDenial ||
            (event.permission == "channels.move" &&
             event.reasonCode == "csrf_validation_failed" &&
             event.outcome == "dispatch_denied");
        sawChannelAllowed = sawChannelAllowed ||
            (event.permission == "channels.move" &&
             event.outcome == "dispatch_authorized");
        assert(event.permission.find("Basic ") == std::string::npos);
        assert(event.reasonCode.find("test-password") ==
            std::string::npos);
        assert(event.reasonCode.find(kBrowserCsrfSecret) ==
            std::string::npos);
        assert(event.reasonCode.find(kWrongBrowserCsrfSecret) ==
            std::string::npos);
        assert(event.reasonCode.find(kBrowserSessionSecret) ==
            std::string::npos);
    }
    assert(sawAllowed);
    assert(sawDenied);
    assert(sawUnmigratedDenial);
    assert(sawCsrfDenial);
    assert(sawTimerCsrfDenial);
    assert(sawTimerAllowed);
    assert(sawChannelCsrfDenial);
    assert(sawChannelAllowed);

    assert(browserRepository.revokeBySessionId(
        "session-browser-active"));
    const SecurityGateDecision revokedBrowserNoFallback =
        compatibilityGate.evaluate(
            browserGetRequest(
                validBrowserCookie,
                kLegacyCredential));
    assert(!revokedBrowserNoFallback.allowed);
    assert(revokedBrowserNoFallback.rejection.statusCode == 401);
    assert(revokedBrowserNoFallback.rejection.body.find(
        "credential_revoked") != std::string::npos);

    assert(identityRepository.revokeCredential(managed.credentialId));
    const SecurityGateDecision revokedManagedCredential =
        compatibilityGate.evaluate(
            remoteRequest("default", kManagedCredential));
    assert(!revokedManagedCredential.allowed);
    assert(revokedManagedCredential.rejection.statusCode == 401);

    assert(identityRepository.revokeCredential(
        "legacy-basic-credential"));
    const SecurityGateDecision revokedLegacyCredential =
        compatibilityGate.evaluate(
            remoteRequest("default", kLegacyCredential));
    assert(!revokedLegacyCredential.allowed);
    assert(revokedLegacyCredential.rejection.statusCode == 401);

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
