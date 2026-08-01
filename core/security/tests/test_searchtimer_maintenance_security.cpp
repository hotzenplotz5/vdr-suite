#include "AccountabilityEventRepository.h"
#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "Database.h"
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
const std::string kSessionSecret =
    "session-secret-0123456789abcdef0123456789";
const std::string kCsrfSecret =
    "csrf-secret-0123456789abcdef012345678901";
const std::string kSessionSecretHash =
    "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
const std::string kCsrfSecretHash =
    "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";

struct Route
{
    std::string path;
    std::string permission;
};

const std::vector<Route> kRoutes = {
    {"/api/searchtimers/update", "searchtimers.modify"},
    {"/api/vdr/searchtimers/update", "searchtimers.modify"},
    {"/api/searchtimers/delete", "searchtimers.delete"},
    {"/api/vdr/searchtimers/delete", "searchtimers.delete"}
};

SecurityConfiguration compatibility()
{
    SecurityConfiguration configuration;
    configuration.mode =
        SecurityMode::LegacyBasicCompatibility;
    configuration.expectedAuthorizationHeader =
        kLegacyCredential;
    configuration.grants = {
        PermissionGrant{"*", "*"}
    };
    return configuration;
}

HttpServerRequest mutationRequest(
    const std::string& path,
    const std::string& backendId,
    bool includeBackendId = true)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = path;
    request.body = includeBackendId
        ? "{\"backendId\":\"" + backendId +
              "\",\"operationId\":\"searchtimer-maintenance-op\"}"
        : "{\"operationId\":\"searchtimer-maintenance-op\"}";
    request.headers["X-Request-ID"] =
        "searchtimer-maintenance-request";
    request.headers["X-Correlation-ID"] =
        "searchtimer-maintenance-correlation";
    return request;
}

void addBrowserCredentials(
    HttpServerRequest& request,
    const std::string& cookie,
    bool csrf)
{
    request.headers["Cookie"] = cookie;
    if (csrf)
    {
        request.headers["X-CSRF-Token"] =
            kCsrfSecret;
    }
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    AccountabilityEventRepository accountabilityRepository(
        database);
    assert(accountabilityRepository.ensureSchema());

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());

    const SecurityConfiguration configuration =
        compatibility();
    assert(identityRepository.ensureCompatibilityIdentity(
        configuration.actorId,
        ActorType::User,
        configuration.actorDisplayName,
        configuration.deviceId,
        configuration.sessionId,
        configuration.credentialId));

    const std::string actorId =
        "phase62-searchtimer-maintenance-actor";
    const std::string deviceId =
        "phase62-searchtimer-maintenance-device";
    const std::string sessionId =
        "phase62-searchtimer-maintenance-session";
    const std::string credentialId =
        "phase62-searchtimer-maintenance-credential";

    SecurityIdentityProvisioningRepository provisioningRepository(
        database);
    assert(provisioningRepository.ensureIdentity(
        actorId,
        ActorType::User,
        "Phase 62 SearchTimer maintenance actor",
        deviceId,
        "Browser test device",
        sessionId,
        credentialId,
        "browser-session"));

    BrowserSessionCredentialRepository browserRepository(
        database);
    assert(browserRepository.ensureSchema());

    SecurityPermissionGrantRepository grantRepository(
        database);
    assert(grantRepository.ensureSchema());

    BrowserSessionCredentialRegistration registration;
    registration.tokenId =
        "searchtimermaintenancetoken";
    registration.actorId = actorId;
    registration.deviceId = deviceId;
    registration.sessionId = sessionId;
    registration.credentialId = credentialId;
    registration.issuedFromCredentialId = credentialId;
    registration.sessionSecretHash =
        kSessionSecretHash;
    registration.csrfSecretHash =
        kCsrfSecretHash;
    registration.expiresAt =
        "2099-01-01 00:00:00";
    assert(browserRepository.insert(registration));

    BrowserSessionAuthenticator browserAuthenticator(
        browserRepository,
        grantRepository);
    PersistentIdentityResolver identityResolver(
        identityRepository);
    SecurityHttpGate gate(
        configuration,
        accountabilityRepository,
        &identityResolver,
        nullptr,
        &browserAuthenticator);

    const std::string cookie =
        "vdr_suite_session=" + registration.tokenId +
        "." + kSessionSecret;

    for (const Route& route : kRoutes)
    {
        HttpServerRequest legacy =
            mutationRequest(route.path, "default");
        legacy.headers["Authorization"] =
            kLegacyCredential;

        const SecurityGateDecision legacyDecision =
            gate.evaluate(legacy);
        assert(legacyDecision.allowed);
        assert(legacyDecision.protectedMutation);

        HttpServerRequest missingCsrf =
            mutationRequest(route.path, "default");
        addBrowserCredentials(
            missingCsrf,
            cookie,
            false);

        const SecurityGateDecision missingCsrfDecision =
            gate.evaluate(missingCsrf);
        assert(!missingCsrfDecision.allowed);
        assert(missingCsrfDecision.protectedMutation);
        assert(
            missingCsrfDecision.rejection.statusCode ==
            403);
        assert(missingCsrfDecision.rejection.body.find(
            "csrf_validation_failed") !=
            std::string::npos);

        HttpServerRequest noPermission =
            mutationRequest(route.path, "default");
        addBrowserCredentials(
            noPermission,
            cookie,
            true);

        const SecurityGateDecision noPermissionDecision =
            gate.evaluate(noPermission);
        assert(!noPermissionDecision.allowed);
        assert(
            noPermissionDecision.rejection.statusCode ==
            403);
        assert(noPermissionDecision.rejection.body.find(
            "permission_denied") !=
            std::string::npos);

        assert(grantRepository.ensureGrant(
            actorId,
            route.permission,
            "default"));

        const SecurityGateDecision directAllowed =
            gate.evaluate(noPermission);
        assert(directAllowed.allowed);
        assert(directAllowed.protectedMutation);

        HttpServerRequest defaultBackend =
            mutationRequest(
                route.path,
                "",
                false);
        addBrowserCredentials(
            defaultBackend,
            cookie,
            true);
        assert(gate.evaluate(defaultBackend).allowed);

        HttpServerRequest query =
            mutationRequest(
                route.path + "?source=browser",
                "default");
        addBrowserCredentials(query, cookie, true);
        assert(gate.evaluate(query).allowed);

        HttpServerRequest wrongScope =
            mutationRequest(route.path, "house-b");
        addBrowserCredentials(
            wrongScope,
            cookie,
            true);

        const SecurityGateDecision wrongScopeDecision =
            gate.evaluate(wrongScope);
        assert(!wrongScopeDecision.allowed);
        assert(
            wrongScopeDecision.rejection.statusCode ==
            403);
        assert(wrongScopeDecision.rejection.body.find(
            "backend_scope_denied") !=
            std::string::npos);

        assert(grantRepository.revokeGrant(
            actorId,
            route.permission,
            "default"));

        HttpServerRequest trailingSlash =
            mutationRequest(
                route.path + "/",
                "default");
        addBrowserCredentials(
            trailingSlash,
            cookie,
            true);

        const SecurityGateDecision trailingSlashDecision =
            gate.evaluate(trailingSlash);
        assert(!trailingSlashDecision.allowed);
        assert(
            trailingSlashDecision.rejection.statusCode ==
            503);
        assert(trailingSlashDecision.rejection.body.find(
            "security_policy_not_migrated") !=
            std::string::npos);
    }

    assert(grantRepository.ensureGrant(
        actorId,
        "role.admin",
        "*"));

    for (const Route& route : kRoutes)
    {
        HttpServerRequest wildcardAdmin =
            mutationRequest(route.path, "default");
        addBrowserCredentials(
            wildcardAdmin,
            cookie,
            true);

        const SecurityGateDecision decision =
            gate.evaluate(wildcardAdmin);
        assert(!decision.allowed);
        assert(decision.rejection.body.find(
            "backend_scope_denied") !=
            std::string::npos);
    }

    assert(grantRepository.revokeGrant(
        actorId,
        "role.admin",
        "*"));
    assert(grantRepository.ensureGrant(
        actorId,
        "role.admin",
        "default"));

    for (const Route& route : kRoutes)
    {
        HttpServerRequest admin =
            mutationRequest(route.path, "default");
        addBrowserCredentials(admin, cookie, true);
        assert(gate.evaluate(admin).allowed);
    }

    assert(grantRepository.ensureGrant(
        actorId,
        "role.read-only",
        "default"));

    for (const Route& route : kRoutes)
    {
        HttpServerRequest readOnly =
            mutationRequest(route.path, "default");
        addBrowserCredentials(
            readOnly,
            cookie,
            true);

        const SecurityGateDecision decision =
            gate.evaluate(readOnly);
        assert(!decision.allowed);
        assert(
            decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "role_read_only") !=
            std::string::npos);
    }

    bool sawModifyCsrf = false;
    bool sawModifyAllowed = false;
    bool sawDeleteCsrf = false;
    bool sawDeleteAllowed = false;

    for (const AccountabilityEvent& event :
         accountabilityRepository.listAll())
    {
        sawModifyCsrf = sawModifyCsrf ||
            (event.permission ==
                 "searchtimers.modify" &&
             event.reasonCode ==
                 "csrf_validation_failed" &&
             event.outcome ==
                 "dispatch_denied");
        sawModifyAllowed = sawModifyAllowed ||
            (event.permission ==
                 "searchtimers.modify" &&
             event.outcome ==
                 "dispatch_authorized");
        sawDeleteCsrf = sawDeleteCsrf ||
            (event.permission ==
                 "searchtimers.delete" &&
             event.reasonCode ==
                 "csrf_validation_failed" &&
             event.outcome ==
                 "dispatch_denied");
        sawDeleteAllowed = sawDeleteAllowed ||
            (event.permission ==
                 "searchtimers.delete" &&
             event.outcome ==
                 "dispatch_authorized");

        assert(event.permission.find("Basic ") ==
            std::string::npos);
        assert(event.reasonCode.find(kSessionSecret) ==
            std::string::npos);
        assert(event.reasonCode.find(kCsrfSecret) ==
            std::string::npos);
    }

    assert(sawModifyCsrf);
    assert(sawModifyAllowed);
    assert(sawDeleteCsrf);
    assert(sawDeleteAllowed);

    return 0;
}
