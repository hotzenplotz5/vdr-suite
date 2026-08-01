#pragma once

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

class SecurityHttpGateBrowserTestFixture
{
public:
    static inline const std::string legacyCredential =
        "Basic YWRtaW46dmRyLXN1aXRl";
    static inline const std::string sessionSecret =
        "session-secret-0123456789abcdef0123456789";
    static inline const std::string csrfSecret =
        "csrf-secret-0123456789abcdef012345678901";

    static SecurityConfiguration configuration()
    {
        SecurityConfiguration value;
        value.mode = SecurityMode::LegacyBasicCompatibility;
        value.expectedAuthorizationHeader = legacyCredential;
        value.grants = {
            PermissionGrant{"*", "*"}
        };
        return value;
    }

    SecurityHttpGateBrowserTestFixture()
        : accountabilityRepository(database),
          identityRepository(database),
          provisioningRepository(database),
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
              nullptr,
              &browserAuthenticator)
    {
        assert(database.open(":memory:"));
        assert(accountabilityRepository.ensureSchema());
        assert(identityRepository.ensureSchema());
        assert(browserRepository.ensureSchema());
        assert(grantRepository.ensureSchema());

        const SecurityConfiguration legacy =
            configuration();
        assert(identityRepository.ensureCompatibilityIdentity(
            legacy.actorId,
            ActorType::User,
            legacy.actorDisplayName,
            legacy.deviceId,
            legacy.sessionId,
            legacy.credentialId));

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

    void addLegacyAuthentication(
        HttpServerRequest& request) const
    {
        request.headers["Authorization"] =
            legacyCredential;
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
