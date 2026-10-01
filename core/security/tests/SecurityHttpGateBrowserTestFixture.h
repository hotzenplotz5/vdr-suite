#pragma once

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

class SecurityHttpGateBrowserTestFixture
{
public:
    static inline const std::string managedCredential =
        "Basic cGhhc2U2Mi1hZG1pbjp0ZXN0LXBhc3N3b3Jk";
    static inline const std::string managedPasswordHash =
        "$6$testsalt$qzmynZ3SU0S5D.QBAsFplf6HVa.jpeEdx88KlHvhGfddFSPHoEWMArwiVQ1PLzZDrJJ9Vs/zKBgHPMSwmFddx.";
    static inline const std::string sessionSecret =
        "session-secret-0123456789abcdef0123456789";
    static inline const std::string csrfSecret =
        "csrf-secret-0123456789abcdef012345678901";

    static SecurityConfiguration configuration()
    {
        return SecurityConfiguration{};
    }

    static ManagedBasicConfiguration managedConfiguration()
    {
        ManagedBasicConfiguration value;
        value.username = "phase62-admin";
        value.passwordHash = managedPasswordHash;
        value.actorId = "phase62-managed-admin";
        value.actorDisplayName = "Phase 62 managed administrator";
        value.deviceId = "phase62-managed-admin-client";
        value.sessionId = "phase62-managed-admin-session";
        value.credentialId = "phase62-managed-admin-credential";
        value.grants = {PermissionGrant{"*", "*"}};
        return value;
    }

    SecurityHttpGateBrowserTestFixture()
        : accountabilityRepository(database),
          identityRepository(database),
          provisioningRepository(database),
          verifierRepository(database),
          managedBasic(managedConfiguration()),
          managedAuthenticator(managedBasic, verifierRepository),
          browserRepository(database),
          grantRepository(database),
          browserAuthenticator(
              browserRepository,
              grantRepository),
          identityResolver(identityRepository),
          gate(
              configuration(),
              accountabilityRepository,
              &identityResolver,
              &managedAuthenticator,
              &browserAuthenticator)
    {
        assert(database.open(":memory:"));
        assert(accountabilityRepository.ensureSchema());
        assert(identityRepository.ensureSchema());
        assert(browserRepository.ensureSchema());
        assert(grantRepository.ensureSchema());

        assert(verifierRepository.ensureSchema());
        assert(provisioningRepository.ensureIdentity(
            managedBasic.actorId,
            ActorType::User,
            managedBasic.actorDisplayName,
            managedBasic.deviceId,
            "Managed Basic client",
            managedBasic.sessionId,
            managedBasic.credentialId,
            "managed-basic"));
        assert(verifierRepository.ensureVerifier(
            managedBasic.credentialId,
            managedBasic.username,
            managedPasswordHash));

        assert(provisioningRepository.ensureIdentity(
            actorId,
            ActorType::User,
            "Phase 62 browser test actor",
            deviceId,
            "Phase 62 browser test device",
            sessionId,
            credentialId,
            "browser-session"));

        BrowserSessionCredentialRegistration registration;
        registration.tokenId = tokenId;
        registration.actorId = actorId;
        registration.deviceId = deviceId;
        registration.sessionId = sessionId;
        registration.credentialId = credentialId;
        registration.issuedFromCredentialId = credentialId;
        registration.sessionSecretHash =
            "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
        registration.csrfSecretHash =
            "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";
        registration.expiresAt =
            "2099-01-01 00:00:00";
        assert(browserRepository.insert(registration));

        cookie =
            "vdr_suite_session=" + tokenId +
            "." + sessionSecret;
    }

    HttpServerRequest mutationRequest(
        const std::string& path,
        const std::string& backendId,
        bool includeBackendId = true) const
    {
        HttpServerRequest request;
        request.method = "POST";
        request.path = path;
        request.body = includeBackendId
            ? "{\"backendId\":\"" + backendId +
                  "\",\"operationId\":\"phase62-test-operation\"}"
            : "{\"operationId\":\"phase62-test-operation\"}";
        request.headers["X-Request-ID"] =
            "phase62-test-request";
        request.headers["X-Correlation-ID"] =
            "phase62-test-correlation";
        return request;
    }

    void addManagedBasicAuthentication(
        HttpServerRequest& request) const
    {
        request.headers["Authorization"] =
            managedCredential;
    }

    void addBrowserAuthentication(
        HttpServerRequest& request,
        bool includeCsrf = false) const
    {
        request.headers["Cookie"] = cookie;
        if (includeCsrf)
        {
            request.headers["X-CSRF-Token"] =
                csrfSecret;
        }
    }

    Database database;
    AccountabilityEventRepository accountabilityRepository;
    SecurityIdentityRepository identityRepository;
    SecurityIdentityProvisioningRepository provisioningRepository;
    CredentialVerifierRepository verifierRepository;
    ManagedBasicConfiguration managedBasic;
    ManagedBasicAuthenticator managedAuthenticator;
    BrowserSessionCredentialRepository browserRepository;
    SecurityPermissionGrantRepository grantRepository;
    BrowserSessionAuthenticator browserAuthenticator;
    PersistentIdentityResolver identityResolver;
    SecurityHttpGate gate;

    const std::string actorId =
        "phase62-browser-test-actor";
    const std::string deviceId =
        "phase62-browser-test-device";
    const std::string sessionId =
        "phase62-browser-test-session";
    const std::string credentialId =
        "phase62-browser-test-credential";
    const std::string tokenId =
        "phase62browsertesttoken";
    std::string cookie;
};
