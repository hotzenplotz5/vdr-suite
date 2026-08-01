#include "AccountabilityEventRepository.h"
#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionHttpService.h"
#include "BrowserSessionIssuanceService.h"
#include "BrowserSessionLifecycleService.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{
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

std::string prefixedHex(
    const std::string& prefix,
    const std::vector<unsigned char>& value)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result = prefix;
    for (const unsigned char byte : value)
    {
        result.push_back(Hex[(byte >> 4) & 0x0f]);
        result.push_back(Hex[byte & 0x0f]);
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

BrowserSessionIssuanceService::Clock fixedClock()
{
    return []
    {
        return std::chrono::system_clock::time_point(
            std::chrono::seconds(4070908800));
    };
}

std::string jsonStringValue(
    const std::string& body,
    const std::string& field)
{
    const std::string prefix = "\"" + field + "\":\"";
    const std::size_t begin = body.find(prefix);
    if (begin == std::string::npos)
    {
        return {};
    }
    const std::size_t valueBegin = begin + prefix.size();
    const std::size_t end = body.find('"', valueBegin);
    return end == std::string::npos
        ? std::string()
        : body.substr(valueBegin, end - valueBegin);
}

std::string cookiePair(const std::string& setCookie)
{
    const std::size_t separator = setCookie.find(';');
    return separator == std::string::npos
        ? setCookie
        : setCookie.substr(0, separator);
}

bool hasOutcome(
    const std::vector<AccountabilityEvent>& events,
    const std::string& eventType,
    const std::string& permission,
    const std::string& action,
    const std::string& reasonCode,
    const std::string& outcome,
    const std::string& requestId,
    const std::string& sessionId = {})
{
    return std::any_of(
        events.begin(),
        events.end(),
        [&](const AccountabilityEvent& event)
        {
            return event.eventType == eventType &&
                event.permission == permission &&
                event.action == action &&
                event.decision == "allowed" &&
                event.reasonCode == reasonCode &&
                event.outcome == outcome &&
                event.backendId == "*" &&
                event.requestId == requestId &&
                (sessionId.empty() || event.sessionId == sessionId);
        });
}

bool eventContains(
    const AccountabilityEvent& event,
    const std::string& value)
{
    if (value.empty())
    {
        return false;
    }

    const std::vector<std::string> fields = {
        event.eventId,
        event.classes,
        event.eventType,
        event.severity,
        event.occurredAt,
        event.actorId,
        event.actorType,
        event.deviceId,
        event.sessionId,
        event.authenticationState,
        event.permission,
        event.backendId,
        event.operationId,
        event.requestId,
        event.correlationId,
        event.action,
        event.decision,
        event.reasonCode,
        event.outcome,
    };

    return std::any_of(
        fields.begin(),
        fields.end(),
        [&](const std::string& field)
        {
            return field.find(value) != std::string::npos;
        });
}

void createAccountabilityBlock(Database& database)
{
    assert(database.execute(
        "CREATE TRIGGER phase62_test_accountability_block "
        "BEFORE INSERT ON accountability_events "
        "BEGIN SELECT RAISE(ABORT, 'phase62 test block'); END;"));
}

void dropAccountabilityBlock(Database& database)
{
    assert(database.execute(
        "DROP TRIGGER phase62_test_accountability_block;"));
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
    assert(provisioningRepository.ensureIdentity(
        "user-phase62-admin",
        ActorType::User,
        "Phase 62 administrator",
        "device-phase62-browser",
        "Phase 62 browser",
        "session-phase62-source",
        "credential-phase62-admin",
        "managed-basic"));

    BrowserSessionCredentialRepository credentialRepository(database);
    assert(credentialRepository.ensureSchema());

    SecurityPermissionGrantRepository permissionGrantRepository(database);
    assert(permissionGrantRepository.ensureSchema());

    const auto tokenBytes = bytes(0x10, 16);
    const auto sessionBytes = bytes(0x20, 16);
    const auto credentialBytes = bytes(0x30, 16);

    BrowserSessionIssuanceService issuanceService(
        database,
        identityRepository,
        credentialRepository,
        sequenceEntropy({
            tokenBytes,
            sessionBytes,
            credentialBytes,
            bytes(0x40, 32),
            bytes(0x60, 32),
            bytes(0x80, 16),
            bytes(0xa0, 16),
        }),
        fixedClock());
    BrowserSessionLifecycleService lifecycleService(
        database,
        identityRepository,
        credentialRepository);

    BrowserSessionLifetimeConfiguration lifetimeConfiguration;
    lifetimeConfiguration.seconds = 900;
    BrowserSessionHttpService httpService(
        issuanceService,
        lifecycleService,
        accountabilityRepository,
        lifetimeConfiguration);

    RequestSecurityContext basicContext;
    basicContext.actor = ActorIdentity{
        "user-phase62-admin",
        ActorType::User,
        "Phase 62 administrator",
        true};
    basicContext.device = DeviceIdentity{
        "device-phase62-browser",
        true};
    basicContext.session = SessionIdentity{
        "session-phase62-source",
        true,
        false,
        false};
    basicContext.credential = CredentialIdentity{
        "credential-phase62-admin",
        true,
        false,
        false};
    basicContext.authenticationState =
        AuthenticationState::Authenticated;
    basicContext.requestId = "request-browser-login";

    const HttpServerResponse login = httpService.login(basicContext);
    assert(login.statusCode == 200);
    assert(login.headers.at("Content-Type") == "application/json");
    assert(login.headers.at("Cache-Control") == "no-store");
    assert(login.headers.at("Pragma") == "no-cache");

    const std::string setCookie = login.headers.at("Set-Cookie");
    assert(setCookie.rfind("vdr_suite_session=", 0) == 0);
    assert(setCookie.find("; Path=/") != std::string::npos);
    assert(setCookie.find("; Max-Age=900") != std::string::npos);
    assert(setCookie.find("; Max-Age=28800") == std::string::npos);
    assert(setCookie.find("; HttpOnly") != std::string::npos);
    assert(setCookie.find("; Secure") != std::string::npos);
    assert(setCookie.find("; SameSite=Strict") != std::string::npos);
    assert(setCookie.find("Domain=") == std::string::npos);

    const std::string cookie = cookiePair(setCookie);
    const std::string cookieValue = cookie.substr(
        std::string("vdr_suite_session=").size());
    const std::string csrfToken =
        jsonStringValue(login.body, "csrfToken");
    assert(!cookieValue.empty());
    assert(!csrfToken.empty());
    assert(login.body.find(cookieValue) == std::string::npos);
    assert(login.body.find("vdr_suite_session") == std::string::npos);
    assert(login.body.find("2099-01-01 00:15:00") != std::string::npos);
    assert(login.body.find("request-browser-login") != std::string::npos);

    const std::string tokenId = prefixedHex("bst_", tokenBytes);
    const std::string sessionId = prefixedHex("bss_", sessionBytes);
    const std::string credentialId = prefixedHex(
        "bsc_",
        credentialBytes);
    const auto stored = credentialRepository.findByTokenId(tokenId);
    assert(stored.has_value());
    assert(stored->sessionId == sessionId);
    assert(stored->credentialId == credentialId);
    assert(stored->expiresAt == "2099-01-01 00:15:00");
    assert(stored->sessionSecretHash != cookieValue);
    assert(stored->csrfSecretHash != csrfToken);

    assert(hasOutcome(
        accountabilityRepository.listAll(),
        "operation.succeeded",
        "session.issue.self",
        "browser.session.issue",
        "browser_session_issued",
        "succeeded",
        "request-browser-login",
        sessionId));

    BrowserSessionAuthenticator authenticator(
        credentialRepository,
        permissionGrantRepository);
    const std::map<std::string, std::string> logoutHeaders = {
        {"Cookie", cookie},
        {"X-CSRF-Token", csrfToken},
    };
    RequestSecurityContext browserContext = authenticator.authenticate(
        logoutHeaders,
        "request-browser-logout",
        "correlation-browser-lifecycle");
    assert(browserContext.authenticated());
    assert(browserContext.permissionGrantResolution ==
        PermissionGrantResolutionState::Resolved);
    assert(browserContext.grants.empty());
    assert(authenticator.verifyCsrf(logoutHeaders));

    const HttpServerResponse logout = httpService.logout(browserContext);
    assert(logout.statusCode == 204);
    assert(logout.body.empty());
    assert(logout.headers.at("Cache-Control") == "no-store");
    assert(logout.headers.at("Pragma") == "no-cache");
    const std::string clearedCookie = logout.headers.at("Set-Cookie");
    assert(clearedCookie.rfind("vdr_suite_session=;", 0) == 0);
    assert(clearedCookie.find("Max-Age=0") != std::string::npos);
    assert(clearedCookie.find("Expires=Thu, 01 Jan 1970 00:00:00 GMT") !=
        std::string::npos);
    assert(clearedCookie.find("HttpOnly") != std::string::npos);
    assert(clearedCookie.find("Secure") != std::string::npos);
    assert(clearedCookie.find("SameSite=Strict") != std::string::npos);

    const auto revokedBrowser =
        credentialRepository.findBySessionId(sessionId);
    const auto revokedSession = identityRepository.findSession(sessionId);
    const auto revokedCredential =
        identityRepository.findCredential(credentialId);
    assert(revokedBrowser.has_value());
    assert(!revokedBrowser->active && revokedBrowser->revoked);
    assert(revokedSession.has_value());
    assert(!revokedSession->active && revokedSession->revoked);
    assert(revokedCredential.has_value());
    assert(!revokedCredential->active && revokedCredential->revoked);
    assert(authenticator.authenticate(
        logoutHeaders,
        "request-after-logout",
        "").authenticationState == AuthenticationState::Revoked);

    assert(hasOutcome(
        accountabilityRepository.listAll(),
        "operation.succeeded",
        "session.revoke.self",
        "browser.session.revoke",
        "browser_session_revoked",
        "succeeded",
        "request-browser-logout",
        sessionId));

    RequestSecurityContext invalidContext;
    invalidContext.requestId = "request-invalid-login";
    const HttpServerResponse invalidLogin =
        httpService.login(invalidContext);
    assert(invalidLogin.statusCode == 401);
    assert(invalidLogin.headers.find("Set-Cookie") ==
        invalidLogin.headers.end());
    assert(invalidLogin.body.find("invalid_session_issuance_context") !=
        std::string::npos);

    BrowserSessionLifetimeConfiguration invalidLifetimeConfiguration;
    invalidLifetimeConfiguration.configuredValueValid = false;
    BrowserSessionHttpService invalidLifetimeService(
        issuanceService,
        lifecycleService,
        accountabilityRepository,
        invalidLifetimeConfiguration);
    basicContext.requestId = "request-invalid-lifetime";
    const HttpServerResponse invalidLifetimeLogin =
        invalidLifetimeService.login(basicContext);
    assert(invalidLifetimeLogin.statusCode == 503);
    assert(invalidLifetimeLogin.headers.find("Set-Cookie") ==
        invalidLifetimeLogin.headers.end());
    assert(invalidLifetimeLogin.headers.at("Cache-Control") == "no-store");
    assert(invalidLifetimeLogin.body.find(
        "browser_session_lifetime_configuration_invalid") !=
        std::string::npos);
    assert(invalidLifetimeLogin.body.find("request-invalid-lifetime") !=
        std::string::npos);
    assert(hasOutcome(
        accountabilityRepository.listAll(),
        "operation.failed",
        "session.issue.self",
        "browser.session.issue",
        "browser_session_lifetime_configuration_invalid",
        "failed",
        "request-invalid-lifetime"));

    BrowserSessionLifetimeConfiguration belowMinimumConfiguration;
    belowMinimumConfiguration.seconds = 299;
    BrowserSessionHttpService belowMinimumService(
        issuanceService,
        lifecycleService,
        accountabilityRepository,
        belowMinimumConfiguration);
    basicContext.requestId = "request-below-minimum";
    const HttpServerResponse belowMinimumLogin =
        belowMinimumService.login(basicContext);
    assert(belowMinimumLogin.statusCode == 503);
    assert(belowMinimumLogin.headers.find("Set-Cookie") ==
        belowMinimumLogin.headers.end());
    assert(hasOutcome(
        accountabilityRepository.listAll(),
        "operation.failed",
        "session.issue.self",
        "browser.session.issue",
        "browser_session_lifetime_configuration_invalid",
        "failed",
        "request-below-minimum"));

    BrowserSessionIssuanceService failingIssuanceService(
        database,
        identityRepository,
        credentialRepository,
        [](unsigned char*, std::size_t)
        {
            return false;
        },
        fixedClock());
    BrowserSessionHttpService failingIssueHttpService(
        failingIssuanceService,
        lifecycleService,
        accountabilityRepository,
        lifetimeConfiguration);
    basicContext.requestId = "request-issuance-failure";
    const HttpServerResponse issuanceFailure =
        failingIssueHttpService.login(basicContext);
    assert(issuanceFailure.statusCode == 503);
    assert(issuanceFailure.headers.find("Set-Cookie") ==
        issuanceFailure.headers.end());
    assert(issuanceFailure.body.find("browser_session_issuance_failed") !=
        std::string::npos);
    assert(hasOutcome(
        accountabilityRepository.listAll(),
        "operation.failed",
        "session.issue.self",
        "browser.session.issue",
        "browser_session_issuance_failed",
        "failed",
        "request-issuance-failure"));

    browserContext.requestId = "request-revocation-failure";
    const HttpServerResponse revocationFailure =
        httpService.logout(browserContext);
    assert(revocationFailure.statusCode == 503);
    assert(revocationFailure.headers.find("Set-Cookie") ==
        revocationFailure.headers.end());
    assert(revocationFailure.body.find(
        "browser_session_revocation_failed") != std::string::npos);
    assert(hasOutcome(
        accountabilityRepository.listAll(),
        "operation.failed",
        "session.revoke.self",
        "browser.session.revoke",
        "browser_session_revocation_failed",
        "failed",
        "request-revocation-failure",
        sessionId));

    const auto compensatedTokenBytes = bytes(0xb0, 16);
    const auto compensatedSessionBytes = bytes(0xc0, 16);
    const auto compensatedCredentialBytes = bytes(0xd0, 16);
    BrowserSessionIssuanceService compensatedIssuanceService(
        database,
        identityRepository,
        credentialRepository,
        sequenceEntropy({
            compensatedTokenBytes,
            compensatedSessionBytes,
            compensatedCredentialBytes,
            bytes(0x11, 32),
            bytes(0x31, 32),
            bytes(0x51, 16),
            bytes(0x71, 16),
        }),
        fixedClock());
    BrowserSessionHttpService compensatedHttpService(
        compensatedIssuanceService,
        lifecycleService,
        accountabilityRepository,
        lifetimeConfiguration);

    createAccountabilityBlock(database);
    basicContext.requestId = "request-outcome-blocked-login";
    const HttpServerResponse blockedLogin =
        compensatedHttpService.login(basicContext);
    assert(blockedLogin.statusCode == 503);
    assert(blockedLogin.headers.find("Set-Cookie") ==
        blockedLogin.headers.end());
    assert(blockedLogin.body.find("accountability_unavailable") !=
        std::string::npos);
    dropAccountabilityBlock(database);

    const std::string compensatedTokenId = prefixedHex(
        "bst_",
        compensatedTokenBytes);
    const std::string compensatedSessionId = prefixedHex(
        "bss_",
        compensatedSessionBytes);
    const std::string compensatedCredentialId = prefixedHex(
        "bsc_",
        compensatedCredentialBytes);
    const auto compensatedBrowser =
        credentialRepository.findByTokenId(compensatedTokenId);
    const auto compensatedSession =
        identityRepository.findSession(compensatedSessionId);
    const auto compensatedCredential =
        identityRepository.findCredential(compensatedCredentialId);
    assert(compensatedBrowser.has_value());
    assert(!compensatedBrowser->active && compensatedBrowser->revoked);
    assert(compensatedSession.has_value());
    assert(!compensatedSession->active && compensatedSession->revoked);
    assert(compensatedCredential.has_value());
    assert(!compensatedCredential->active && compensatedCredential->revoked);

    const auto blockedLogoutTokenBytes = bytes(0xe0, 16);
    const auto blockedLogoutSessionBytes = bytes(0xf0, 16);
    const auto blockedLogoutCredentialBytes = bytes(0x01, 16);
    BrowserSessionIssuanceService blockedLogoutIssuanceService(
        database,
        identityRepository,
        credentialRepository,
        sequenceEntropy({
            blockedLogoutTokenBytes,
            blockedLogoutSessionBytes,
            blockedLogoutCredentialBytes,
            bytes(0x21, 32),
            bytes(0x41, 32),
            bytes(0x61, 16),
            bytes(0x81, 16),
        }),
        fixedClock());
    BrowserSessionHttpService blockedLogoutHttpService(
        blockedLogoutIssuanceService,
        lifecycleService,
        accountabilityRepository,
        lifetimeConfiguration);

    basicContext.requestId = "request-before-blocked-logout";
    const HttpServerResponse beforeBlockedLogout =
        blockedLogoutHttpService.login(basicContext);
    assert(beforeBlockedLogout.statusCode == 200);
    const std::string blockedLogoutCookie = cookiePair(
        beforeBlockedLogout.headers.at("Set-Cookie"));
    const std::string blockedLogoutCsrf = jsonStringValue(
        beforeBlockedLogout.body,
        "csrfToken");
    const std::map<std::string, std::string> blockedLogoutHeaders = {
        {"Cookie", blockedLogoutCookie},
        {"X-CSRF-Token", blockedLogoutCsrf},
    };
    RequestSecurityContext blockedLogoutContext =
        authenticator.authenticate(
            blockedLogoutHeaders,
            "request-outcome-blocked-logout",
            "");
    assert(blockedLogoutContext.authenticated());

    createAccountabilityBlock(database);
    const HttpServerResponse blockedLogout =
        blockedLogoutHttpService.logout(blockedLogoutContext);
    assert(blockedLogout.statusCode == 503);
    assert(blockedLogout.body.find("accountability_unavailable") !=
        std::string::npos);
    assert(blockedLogout.headers.at("Set-Cookie").find("Max-Age=0") !=
        std::string::npos);
    dropAccountabilityBlock(database);

    const std::string blockedLogoutSessionId = prefixedHex(
        "bss_",
        blockedLogoutSessionBytes);
    const std::string blockedLogoutCredentialId = prefixedHex(
        "bsc_",
        blockedLogoutCredentialBytes);
    const auto blockedLogoutBrowser =
        credentialRepository.findBySessionId(blockedLogoutSessionId);
    const auto blockedLogoutSession =
        identityRepository.findSession(blockedLogoutSessionId);
    const auto blockedLogoutCredential =
        identityRepository.findCredential(blockedLogoutCredentialId);
    assert(blockedLogoutBrowser.has_value());
    assert(!blockedLogoutBrowser->active && blockedLogoutBrowser->revoked);
    assert(blockedLogoutSession.has_value());
    assert(!blockedLogoutSession->active && blockedLogoutSession->revoked);
    assert(blockedLogoutCredential.has_value());
    assert(!blockedLogoutCredential->active && blockedLogoutCredential->revoked);
    assert(authenticator.authenticate(
        blockedLogoutHeaders,
        "request-blocked-logout-replay",
        "").authenticationState == AuthenticationState::Revoked);

    for (const AccountabilityEvent& event : accountabilityRepository.listAll())
    {
        assert(!eventContains(event, cookieValue));
        assert(!eventContains(event, csrfToken));
        assert(!eventContains(event, blockedLogoutCookie));
        assert(!eventContains(event, blockedLogoutCsrf));
    }

    return 0;
}
