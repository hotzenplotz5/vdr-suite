#include "AccountabilityEventRepository.h"
#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionHttpGate.h"
#include "BrowserSessionHttpService.h"
#include "BrowserSessionIssuanceService.h"
#include "BrowserSessionLifecycleService.h"
#include "Database.h"
#include "SecurityConfiguration.h"
#include "SecurityHttpGate.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <string>

namespace
{
BrowserSessionIssuanceService::EntropySource deterministicEntropy()
{
    return [value = static_cast<unsigned int>(1)](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr || size == 0)
        {
            return false;
        }
        for (std::size_t index = 0; index < size; ++index)
        {
            output[index] = static_cast<unsigned char>(value & 0xffU);
            ++value;
        }
        return true;
    };
}

BrowserSessionIssuanceService::Clock fixedClock()
{
    return []
    {
        return std::chrono::system_clock::time_point(
            std::chrono::seconds(4070908800));
    };
}

void provisionActor(
    SecurityIdentityProvisioningRepository& repository,
    const std::string& suffix)
{
    assert(repository.ensureIdentity(
        "user-" + suffix,
        ActorType::User,
        "User " + suffix,
        "device-" + suffix,
        "Device " + suffix,
        "source-session-" + suffix,
        "source-credential-" + suffix,
        "managed-basic"));
}

RequestSecurityContext basicContext(const std::string& suffix)
{
    RequestSecurityContext context;
    context.actor = ActorIdentity{
        "user-" + suffix,
        ActorType::User,
        "User " + suffix,
        true};
    context.device = DeviceIdentity{
        "device-" + suffix,
        true};
    context.session = SessionIdentity{
        "source-session-" + suffix,
        true,
        false,
        false};
    context.credential = CredentialIdentity{
        "source-credential-" + suffix,
        true,
        false,
        false};
    context.authenticationState = AuthenticationState::Authenticated;
    context.requestId = "request-" + suffix;
    context.correlationId = "correlation-" + suffix;
    return context;
}

std::map<std::string, std::string> browserHeaders(
    const IssuedBrowserSession& issued)
{
    return {
        {"Cookie", "vdr_suite_session=" + issued.sessionCookieValue},
        {"X-CSRF-Token", issued.csrfToken},
    };
}

void setLastSeen(
    Database& database,
    const std::string& tokenId,
    const std::string& expression)
{
    assert(database.execute(
        "UPDATE security_browser_session_credentials "
        "SET last_seen_at = " + expression +
        " WHERE token_id = '" + tokenId + "';"));
}
}

int main()
{
    unsetenv("VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS");
    SecurityConfiguration configuration =
        SecurityConfiguration::fromEnvironment();
    assert(configuration.browserSessionIdle.valid());
    assert(!configuration.browserSessionIdle.enabled());
    assert(configuration.browserSessionIdle.timeoutSeconds == 0);

    setenv("VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS", "300", 1);
    configuration = SecurityConfiguration::fromEnvironment();
    assert(configuration.browserSessionIdle.valid());
    assert(configuration.browserSessionIdle.enabled());
    assert(configuration.browserSessionIdle.timeoutSeconds == 300);

    setenv("VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS", "86400", 1);
    assert(SecurityConfiguration::fromEnvironment().browserSessionIdle.valid());

    for (const char* invalid : {
             "", "-1", "+300", " 300", "300 ", "300s",
             "1", "299", "86401", "999999999999999999999"})
    {
        setenv("VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS", invalid, 1);
        assert(!SecurityConfiguration::fromEnvironment()
                    .browserSessionIdle.valid());
    }
    unsetenv("VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS");

    {
        Database database;
        assert(database.open(":memory:"));
        assert(database.execute(
            "CREATE TABLE security_browser_session_credentials ("
            "token_id TEXT PRIMARY KEY,"
            "session_id TEXT NOT NULL UNIQUE,"
            "actor_id TEXT NOT NULL,"
            "device_id TEXT NOT NULL,"
            "credential_id TEXT NOT NULL UNIQUE,"
            "issued_from_credential_id TEXT NOT NULL,"
            "session_secret_hash TEXT NOT NULL,"
            "csrf_secret_hash TEXT NOT NULL,"
            "active INTEGER NOT NULL DEFAULT 1,"
            "expires_at TEXT NOT NULL,"
            "revoked_at TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");"));
        assert(database.execute(
            "INSERT INTO security_browser_session_credentials ("
            "token_id, session_id, actor_id, device_id, credential_id, "
            "issued_from_credential_id, session_secret_hash, csrf_secret_hash, "
            "expires_at, created_at) VALUES ("
            "'legacy-token', 'legacy-session', 'legacy-actor', "
            "'legacy-device', 'legacy-credential', 'legacy-issuer', "
            "'$6$legacy', '$6$legacy', '2099-01-01 00:00:00', "
            "'2020-01-02 03:04:05');"));

        BrowserSessionCredentialRepository repository(database);
        assert(repository.ensureSchema());
        assert(repository.ensureSchema());
        const auto migrated = repository.findByTokenId("legacy-token");
        assert(migrated.has_value());
        assert(migrated->lastSeenAt == "2020-01-02 03:04:05");
    }

    Database database;
    assert(database.open(":memory:"));

    AccountabilityEventRepository accountabilityRepository(database);
    assert(accountabilityRepository.ensureSchema());

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());

    SecurityIdentityProvisioningRepository provisioningRepository(database);
    provisionActor(provisioningRepository, "idle");

    BrowserSessionCredentialRepository credentialRepository(database);
    assert(credentialRepository.ensureSchema());

    SecurityPermissionGrantRepository grantRepository(database);
    assert(grantRepository.ensureSchema());

    BrowserSessionIssuanceService issuanceService(
        database,
        identityRepository,
        credentialRepository,
        deterministicEntropy(),
        fixedClock());
    BrowserSessionLifecycleService lifecycleService(
        database,
        identityRepository,
        credentialRepository);

    BrowserSessionIssuanceRequest request;
    request.actorId = "user-idle";
    request.deviceId = "device-idle";
    request.issuedFromCredentialId = "source-credential-idle";
    request.lifetimeSeconds = 3600;
    request.maximumActivePerActor = 1;
    request.idleTimeoutSeconds = 300;

    BrowserSessionIssuanceResult issuedResult =
        issuanceService.issueWithPolicy(request);
    assert(issuedResult.status == BrowserSessionIssuanceStatus::Issued);
    assert(issuedResult.session.has_value());
    IssuedBrowserSession issued = std::move(*issuedResult.session);

    const auto initialRow =
        credentialRepository.findByTokenId(issued.tokenId);
    assert(initialRow.has_value());
    assert(!initialRow->lastSeenAt.empty());
    const std::string absoluteExpiry = initialRow->expiresAt;

    BrowserSessionAuthenticator authenticator(
        credentialRepository,
        grantRepository,
        300,
        60);
    const auto headers = browserHeaders(issued);

    RequestSecurityContext active = authenticator.authenticate(
        headers,
        "request-active",
        "correlation-active");
    assert(active.authenticated());
    assert(authenticator.verifyCsrf(headers));

    setLastSeen(
        database,
        issued.tokenId,
        "datetime(CURRENT_TIMESTAMP, '-61 seconds')");
    const std::string beforeTouch =
        credentialRepository.findByTokenId(issued.tokenId)->lastSeenAt;
    RequestSecurityContext touched = authenticator.authenticate(
        headers,
        "request-touch",
        "correlation-touch");
    assert(touched.authenticated());
    const auto afterTouch =
        credentialRepository.findByTokenId(issued.tokenId);
    assert(afterTouch.has_value());
    assert(afterTouch->lastSeenAt > beforeTouch);
    assert(afterTouch->expiresAt == absoluteExpiry);

    RequestSecurityContext throttled = authenticator.authenticate(
        headers,
        "request-throttled",
        "correlation-throttled");
    assert(throttled.authenticated());
    const auto afterThrottled =
        credentialRepository.findByTokenId(issued.tokenId);
    assert(afterThrottled.has_value());
    assert(afterThrottled->lastSeenAt == afterTouch->lastSeenAt);
    assert(afterThrottled->expiresAt == absoluteExpiry);

    setLastSeen(
        database,
        issued.tokenId,
        "datetime(CURRENT_TIMESTAMP, '-301 seconds')");
    const std::string idleTimestamp =
        credentialRepository.findByTokenId(issued.tokenId)->lastSeenAt;
    RequestSecurityContext expired = authenticator.authenticate(
        headers,
        "request-idle-expired",
        "correlation-idle-expired");
    assert(expired.authenticationState == AuthenticationState::Expired);
    assert(expired.session.has_value() && expired.session->expired);
    assert(expired.credential.has_value() && !expired.credential->expired);
    assert(!authenticator.verifyCsrf(headers));
    assert(credentialRepository.findByTokenId(issued.tokenId)->lastSeenAt ==
        idleTimestamp);
    assert(credentialRepository.findByTokenId(issued.tokenId)->expiresAt ==
        absoluteExpiry);

    const auto effectiveAfterIdle =
        credentialRepository.countEffectiveActiveByActorId("user-idle", 300);
    assert(effectiveAfterIdle.has_value());
    assert(*effectiveAfterIdle == 0);

    BrowserSessionIssuanceResult replacementResult =
        issuanceService.issueWithPolicy(request);
    assert(replacementResult.status == BrowserSessionIssuanceStatus::Issued);
    assert(replacementResult.session.has_value());
    IssuedBrowserSession replacement =
        std::move(*replacementResult.session);

    BrowserSessionAuthenticator disabledAuthenticator(
        credentialRepository,
        grantRepository,
        0,
        60);
    setLastSeen(
        database,
        replacement.tokenId,
        "datetime(CURRENT_TIMESTAMP, '-100000 seconds')");
    const std::string disabledTimestamp =
        credentialRepository.findByTokenId(replacement.tokenId)->lastSeenAt;
    RequestSecurityContext disabled = disabledAuthenticator.authenticate(
        browserHeaders(replacement),
        "request-disabled",
        "correlation-disabled");
    assert(disabled.authenticated());
    assert(credentialRepository.findByTokenId(replacement.tokenId)->lastSeenAt ==
        disabledTimestamp);

    BrowserSessionAuthenticator invalidAuthenticator(
        credentialRepository,
        grantRepository,
        -1,
        60);
    RequestSecurityContext invalidPolicy = invalidAuthenticator.authenticate(
        browserHeaders(replacement),
        "request-invalid",
        "correlation-invalid");
    assert(invalidPolicy.authenticated());
    assert(invalidPolicy.permissionGrantResolution ==
        PermissionGrantResolutionState::Unavailable);

    BrowserSessionIssuanceRequest invalidRequest = request;
    invalidRequest.idleTimeoutSeconds = 299;
    assert(issuanceService.issueWithPolicy(invalidRequest).status ==
        BrowserSessionIssuanceStatus::Failed);

    SecurityConfiguration gateConfiguration;
    gateConfiguration.mode = SecurityMode::LegacyBasicCompatibility;
    SecurityHttpGate securityGate(
        gateConfiguration,
        accountabilityRepository,
        nullptr,
        nullptr,
        &authenticator);

    HttpServerRequest getRequest;
    getRequest.method = "GET";
    getRequest.path = "/api/backends";
    getRequest.headers = headers;
    getRequest.headers["X-Request-ID"] = "request-idle-get";
    SecurityGateDecision getDecision = securityGate.evaluate(getRequest);
    assert(!getDecision.allowed);
    assert(getDecision.rejection.statusCode == 401);
    assert(getDecision.rejection.body.find("session_expired") !=
        std::string::npos);

    HttpServerRequest mutationRequest;
    mutationRequest.method = "POST";
    mutationRequest.path = "/api/vdr/remote/actions";
    mutationRequest.body = "{\"backendId\":\"default\"}";
    mutationRequest.headers = headers;
    mutationRequest.headers["X-Request-ID"] = "request-idle-mutation";
    SecurityGateDecision mutationDecision =
        securityGate.evaluate(mutationRequest);
    assert(!mutationDecision.allowed);
    assert(mutationDecision.rejection.statusCode == 401);
    assert(mutationDecision.rejection.body.find("session_expired") !=
        std::string::npos);

    SecurityHttpGate invalidPolicyGate(
        gateConfiguration,
        accountabilityRepository,
        nullptr,
        nullptr,
        &invalidAuthenticator);
    HttpServerRequest invalidPolicyGet;
    invalidPolicyGet.method = "GET";
    invalidPolicyGet.path = "/api/backends";
    invalidPolicyGet.headers = browserHeaders(replacement);
    invalidPolicyGet.headers["X-Request-ID"] = "request-invalid-policy-get";
    SecurityGateDecision invalidPolicyDecision =
        invalidPolicyGate.evaluate(invalidPolicyGet);
    assert(!invalidPolicyDecision.allowed);
    assert(invalidPolicyDecision.rejection.statusCode == 503);
    assert(invalidPolicyDecision.rejection.body.find(
        "permission_grants_unavailable") != std::string::npos);

    SecurityConfiguration lifecycleConfiguration;
    lifecycleConfiguration.browserSessionIdle.timeoutSeconds = 300;
    BrowserSessionHttpGate lifecycleGate(
        lifecycleConfiguration,
        accountabilityRepository,
        credentialRepository,
        grantRepository,
        nullptr,
        nullptr);

    HttpServerRequest logoutRequest;
    logoutRequest.method = "POST";
    logoutRequest.path = "/api/security/browser-sessions/logout";
    logoutRequest.headers = headers;
    logoutRequest.headers["X-Request-ID"] = "request-idle-logout";
    BrowserSessionGateDecision logoutDecision =
        lifecycleGate.evaluate(logoutRequest);
    assert(!logoutDecision.allowed);
    assert(logoutDecision.rejection.statusCode == 401);
    assert(logoutDecision.rejection.body.find("session_expired") !=
        std::string::npos);

    SecurityConfiguration invalidLifecycleConfiguration;
    invalidLifecycleConfiguration.browserSessionIdle.configuredValueValid =
        false;
    BrowserSessionHttpGate invalidLifecycleGate(
        invalidLifecycleConfiguration,
        accountabilityRepository,
        credentialRepository,
        grantRepository,
        nullptr,
        nullptr);

    HttpServerRequest loginRequest;
    loginRequest.method = "POST";
    loginRequest.path = "/api/security/browser-sessions";
    loginRequest.headers["X-Request-ID"] = "request-invalid-idle-login";
    BrowserSessionGateDecision invalidLifecycleDecision =
        invalidLifecycleGate.evaluate(loginRequest);
    assert(!invalidLifecycleDecision.allowed);
    assert(invalidLifecycleDecision.rejection.statusCode == 503);
    assert(invalidLifecycleDecision.rejection.body.find(
        "browser_session_idle_configuration_invalid") != std::string::npos);

    BrowserSessionLifetimeConfiguration lifetime;
    lifetime.seconds = 3600;
    BrowserSessionConcurrencyConfiguration concurrency;
    concurrency.maximumActivePerActor = 1;
    BrowserSessionIdleConfiguration invalidIdle;
    invalidIdle.configuredValueValid = false;
    BrowserSessionHttpService invalidHttpService(
        issuanceService,
        lifecycleService,
        accountabilityRepository,
        lifetime,
        concurrency,
        invalidIdle);
    HttpServerResponse invalidIssue =
        invalidHttpService.login(basicContext("idle"));
    assert(invalidIssue.statusCode == 503);
    assert(invalidIssue.headers.count("Set-Cookie") == 0);
    assert(invalidIssue.body.find(
        "browser_session_idle_configuration_invalid") != std::string::npos);

    setLastSeen(
        database,
        replacement.tokenId,
        "datetime(CURRENT_TIMESTAMP, '-61 seconds')");
    assert(database.execute(
        "CREATE TRIGGER fail_browser_activity "
        "BEFORE UPDATE OF last_seen_at "
        "ON security_browser_session_credentials "
        "BEGIN SELECT RAISE(FAIL, 'forced activity failure'); END;"));

    HttpServerRequest activityGet;
    activityGet.method = "GET";
    activityGet.path = "/api/backends";
    activityGet.headers = browserHeaders(replacement);
    activityGet.headers["X-Request-ID"] = "request-activity-failure";
    SecurityGateDecision activityGetDecision =
        securityGate.evaluate(activityGet);
    assert(!activityGetDecision.allowed);
    assert(activityGetDecision.rejection.statusCode == 503);
    assert(activityGetDecision.rejection.body.find(
        "permission_grants_unavailable") != std::string::npos);

    HttpServerRequest activityLogout;
    activityLogout.method = "POST";
    activityLogout.path = "/api/security/browser-sessions/logout";
    activityLogout.headers = browserHeaders(replacement);
    activityLogout.headers["X-Request-ID"] =
        "request-activity-logout-failure";
    BrowserSessionGateDecision activityLogoutDecision =
        lifecycleGate.evaluate(activityLogout);
    assert(!activityLogoutDecision.allowed);
    assert(activityLogoutDecision.rejection.statusCode == 503);
    assert(activityLogoutDecision.rejection.body.find(
        "browser_session_activity_unavailable") != std::string::npos);

    assert(database.execute("DROP TRIGGER fail_browser_activity;"));

    assert(lifecycleService.revoke(
        replacement.sessionId,
        replacement.credentialId));
    assert(disabledAuthenticator.authenticate(
               browserHeaders(replacement),
               "request-replay",
               "correlation-replay")
               .authenticationState == AuthenticationState::Revoked);

    return 0;
}
