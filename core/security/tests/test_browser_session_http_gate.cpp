#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionHttpGate.h"
#include "BrowserSessionIssuanceService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "ManagedBasicAuthenticator.h"
#include "PersistentIdentityResolver.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
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

std::vector<unsigned char> bytes(
    unsigned char start,
    std::size_t size)
{
    std::vector<unsigned char> result(size);
    for (std::size_t index = 0; index < size; ++index)
    {
        result[index] = static_cast<unsigned char>(start + index);
    }
    return result;
}

BrowserSessionIssuanceService::EntropySource sequenceEntropy(
    std::vector<std::vector<unsigned char>> chunks)
{
    return [chunks = std::move(chunks), index = std::size_t{0}](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr ||
            index >= chunks.size() ||
            chunks[index].size() != size)
        {
            return false;
        }
        std::copy(
            chunks[index].begin(),
            chunks[index].end(),
            output);
        ++index;
        return true;
    };
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

HttpServerRequest loginRequest(const std::string& authorization = "")
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/security/browser-sessions";
    request.headers["X-Request-ID"] = "request-session-login";
    if (!authorization.empty())
    {
        request.headers["Authorization"] = authorization;
    }
    return request;
}

HttpServerRequest logoutRequest(
    const std::string& cookie,
    const std::string& csrfToken = "")
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/security/browser-sessions/logout";
    request.headers["X-Request-ID"] = "request-session-logout";
    if (!cookie.empty())
    {
        request.headers["Cookie"] =
            "vdr_suite_session=" + cookie;
    }
    if (!csrfToken.empty())
    {
        request.headers["X-CSRF-Token"] = csrfToken;
    }
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

    const SecurityConfiguration configuration = compatibility();
    assert(identityRepository.ensureCompatibilityIdentity(
        configuration.actorId,
        ActorType::User,
        configuration.actorDisplayName,
        configuration.deviceId,
        configuration.sessionId,
        configuration.credentialId));

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

    PersistentIdentityResolver identityResolver(identityRepository);

    BrowserSessionHttpGate gate(
        configuration,
        accountabilityRepository,
        browserRepository,
        permissionGrantRepository,
        &identityResolver,
        &managedAuthenticator);

    assert(gate.handles(loginRequest()));
    assert(gate.handles(logoutRequest("missing")));

    HttpServerRequest ordinaryGet;
    ordinaryGet.method = "GET";
    ordinaryGet.path = "/api/backends";
    ordinaryGet.headers["Cookie"] =
        "vdr_suite_session=not-a-real-session";
    assert(!gate.handles(ordinaryGet));

    HttpServerRequest remotePost;
    remotePost.method = "POST";
    remotePost.path = "/api/vdr/remote/actions";
    remotePost.headers["Cookie"] =
        "vdr_suite_session=not-a-real-session";
    assert(!gate.handles(remotePost));

    const BrowserSessionGateDecision anonymousLogin =
        gate.evaluate(loginRequest());
    assert(!anonymousLogin.allowed);
    assert(anonymousLogin.rejection.statusCode == 401);
    assert(anonymousLogin.rejection.headers.count("WWW-Authenticate") == 0);
    assert(anonymousLogin.rejection.body.find(
        "authentication_required") != std::string::npos);

    const BrowserSessionGateDecision wrongPassword =
        gate.evaluate(loginRequest(kManagedWrongCredential));
    assert(!wrongPassword.allowed);
    assert(wrongPassword.rejection.statusCode == 401);
    assert(wrongPassword.rejection.body.find(
        "invalid_credentials") != std::string::npos);
    assert(wrongPassword.rejection.body.find(
        "wrong-password") == std::string::npos);
    assert(wrongPassword.rejection.headers.count(
        "WWW-Authenticate") == 0);
    assert(wrongPassword.rejection.headers.count(
        "Set-Cookie") == 0);

    const BrowserSessionGateDecision managedLogin =
        gate.evaluate(loginRequest(kManagedCredential));
    assert(managedLogin.allowed);
    assert(managedLogin.login);
    assert(!managedLogin.logout);
    assert(managedLogin.context.actor.actorId == managed.actorId);
    assert(managedLogin.context.device.has_value());
    assert(managedLogin.context.credential.has_value());

    BrowserSessionIssuanceService issuanceService(
        database,
        identityRepository,
        browserRepository,
        sequenceEntropy({
            bytes(0x10, 16),
            bytes(0x20, 16),
            bytes(0x30, 16),
            bytes(0x40, 32),
            bytes(0x60, 32),
            bytes(0x80, 16),
            bytes(0xa0, 16),
        }),
        []
        {
            return std::chrono::system_clock::time_point(
                std::chrono::seconds(4070908800));
        });
    BrowserSessionIssuanceRequest issuanceRequest;
    issuanceRequest.actorId = managed.actorId;
    issuanceRequest.deviceId = managed.deviceId;
    issuanceRequest.issuedFromCredentialId = managed.credentialId;
    auto issued = issuanceService.issue(issuanceRequest);
    assert(issued.has_value());

    const BrowserSessionGateDecision missingCsrf =
        gate.evaluate(logoutRequest(issued->sessionCookieValue));
    assert(!missingCsrf.allowed);
    assert(missingCsrf.rejection.statusCode == 403);
    assert(missingCsrf.rejection.body.find(
        "csrf_validation_failed") != std::string::npos);
    assert(missingCsrf.rejection.headers.count("WWW-Authenticate") == 0);

    const BrowserSessionGateDecision validLogout = gate.evaluate(
        logoutRequest(
            issued->sessionCookieValue,
            issued->csrfToken));
    assert(validLogout.allowed);
    assert(!validLogout.login);
    assert(validLogout.logout);
    assert(validLogout.context.actor.actorId == managed.actorId);
    assert(validLogout.context.session.has_value());
    assert(validLogout.context.credential.has_value());

    const BrowserSessionGateDecision basicOnlyLogout =
        gate.evaluate(logoutRequest(""));
    assert(!basicOnlyLogout.allowed);
    assert(basicOnlyLogout.rejection.statusCode == 401);
    assert(basicOnlyLogout.rejection.headers.count("WWW-Authenticate") == 0);

    const auto events = accountabilityRepository.listAll();
    bool sawIssueAllowed = false;
    bool sawRevokeAllowed = false;
    bool sawCsrfDenied = false;
    for (const AccountabilityEvent& event : events)
    {
        sawIssueAllowed = sawIssueAllowed ||
            (event.permission == "session.issue.self" &&
             event.decision == "allowed");
        sawRevokeAllowed = sawRevokeAllowed ||
            (event.permission == "session.revoke.self" &&
             event.decision == "allowed");
        sawCsrfDenied = sawCsrfDenied ||
            event.reasonCode == "csrf_validation_failed";
        assert(event.permission.find("Basic ") == std::string::npos);
        assert(event.reasonCode.find("test-password") ==
            std::string::npos);
    }
    assert(sawIssueAllowed);
    assert(sawRevokeAllowed);
    assert(sawCsrfDenied);

    issued->clearSecrets();
    return 0;
}
